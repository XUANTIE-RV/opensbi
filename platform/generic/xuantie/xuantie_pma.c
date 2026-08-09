/*
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_barrier.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_error.h>
#include <xuantie/xuantie_pma.h>

/*
 * Read a PMA register via miselect/mireg indirect access.
 * @param miselect_val: The miselect value (e.g., XUANTIE_PMA_CSR_PMACFG0)
 * @return: The value read from mireg
 */
static unsigned long xuantie_pma_read(unsigned long miselect_val)
{
	unsigned long val;

	csr_write(CSR_MISELECT, miselect_val);
	val = csr_read(CSR_MIREG);

	return val;
}

/*
 * Write a PMA register via miselect/mireg indirect access.
 * @param miselect_val: The miselect value
 * @param val: The value to write to mireg
 */
static void xuantie_pma_write(unsigned long miselect_val, unsigned long val)
{
	csr_write(CSR_MISELECT, miselect_val);
	csr_write(CSR_MIREG, val);
}

static inline bool not_napot(unsigned long addr, unsigned long size)
{
	return ((size & (size - 1)) || (addr & (size - 1)));
}

static inline bool is_pma_entry_disabled(u8 cfg)
{
	return (cfg & XUANTIE_PMACFG_A_MASK) == XUANTIE_PMACFG_A_OFF;
}

/*
 * Get the configuration byte for a specific PMA entry.
 * pmacfg0 holds entries 0-7, pmacfg2 holds entries 8-15 (RV64).
 */
static u8 get_pma_entry_cfg(unsigned int entry_id)
{
	unsigned long miselect_addr;
	unsigned long pmacfg_val;
	u8 *cfg_bytes;

	if (entry_id < 8)
		miselect_addr = XUANTIE_PMA_CSR_PMACFG0;
	else
		miselect_addr = XUANTIE_PMA_CSR_PMACFG2;

	pmacfg_val = xuantie_pma_read(miselect_addr);
	cfg_bytes = (u8 *)&pmacfg_val;

	return cfg_bytes[entry_id % 8];
}

/*
 * Set the configuration byte for a specific PMA entry.
 * Performs read-modify-write on the appropriate pmacfg register.
 */
static void set_pma_entry_cfg(unsigned int entry_id, u8 flags)
{
	unsigned long miselect_addr;
	unsigned long pmacfg_val;
	u8 *cfg_bytes;

	if (entry_id < 8)
		miselect_addr = XUANTIE_PMA_CSR_PMACFG0;
	else
		miselect_addr = XUANTIE_PMA_CSR_PMACFG2;

	pmacfg_val = xuantie_pma_read(miselect_addr);
	cfg_bytes = (u8 *)&pmacfg_val;
	cfg_bytes[entry_id % 8] = flags;

	xuantie_pma_write(miselect_addr, pmacfg_val);
}

/*
 * Read a pmaaddr register for a given entry.
 * XuanTie pmaaddr stores address[47:2].
 */
static unsigned long get_pma_entry_addr(unsigned int entry_id)
{
	return xuantie_pma_read(XUANTIE_PMA_CSR_PMAADDR0 + entry_id);
}

/*
 * Write a pmaaddr register for a given entry.
 */
static void set_pma_entry_addr(unsigned int entry_id, unsigned long val)
{
	xuantie_pma_write(XUANTIE_PMA_CSR_PMAADDR0 + entry_id, val);
}

/*
 * Decode a NAPOT pmaaddr entry to get start address and size.
 * For NAPOT: pmaaddr = (base >> 2) + (size >> 3) - 1
 * The trailing 1-bits encode the size.
 */
static void decode_napot_entry(unsigned int entry_id, unsigned long *start,
			      unsigned long *size)
{
	unsigned long pmaaddr;
	int k;

	pmaaddr = get_pma_entry_addr(entry_id);
	k = sbi_ffz(pmaaddr);
	*size = 1UL << (k + 3);
	*start = (pmaaddr - ((1UL << k) - 1)) << 2;
}

/*
 * Check if a proposed region overlaps with any active PMA NAPOT entry.
 */
static bool has_pma_napot_overlap(unsigned long start, unsigned long size)
{
	unsigned long _start, _size, _end, end;
	u8 cfg;

	end = start + size - 1;
	for (unsigned int i = 0; i < XUANTIE_MAX_PMA_ENTRIES; i++) {
		cfg = get_pma_entry_cfg(i);
		if ((cfg & XUANTIE_PMACFG_A_MASK) != XUANTIE_PMACFG_A_NAPOT)
			continue;

		decode_napot_entry(i, &_start, &_size);
		_end = _start + _size - 1;

		if (MAX(start, _start) <= MIN(end, _end)) {
			sbi_printf(
				"ERROR %s(): %#lx ~ %#lx overlaps with PMA%d: %#lx ~ %#lx\n",
				__func__, start, end, i, _start, _end);
			return true;
		}
	}

	return false;
}

/*
 * Configure a single TOR PMA entry.
 * For TOR: pmaaddr stores the top address >> 2 (address[47:2]).
 * The entry covers [prev_entry_end, this_entry_top).
 * TOR entries must be contiguous and in order.
 */
static int xuantie_pma_setup_tor(const struct xuantie_pma_region *pma_region,
				 unsigned int entry_id)
{
	unsigned long pmaaddr;
	u8 flags = pma_region->flags;

	/* Ensure TOR mode is set in flags */
	flags = (flags & ~XUANTIE_PMACFG_A_MASK) | XUANTIE_PMACFG_A_TOR;

	/* TOR encoding: pmaaddr = (top_addr >> 2) */
	pmaaddr = (pma_region->pa + pma_region->size) >> 2;

	/* Set cfg first, then addr */
	set_pma_entry_cfg(entry_id, flags);
	set_pma_entry_addr(entry_id, pmaaddr);

	/* Verify the address write */
	if (get_pma_entry_addr(entry_id) != pmaaddr) {
		sbi_printf("ERROR %s(): Failed to write pmaaddr%d "
			   "(wrote %#lx, read back %#lx)\n",
			   __func__, entry_id, pmaaddr,
			   get_pma_entry_addr(entry_id));
		/* Disable the entry on failure */
		set_pma_entry_cfg(entry_id, XUANTIE_PMACFG_A_OFF);
		return SBI_EINVAL;
	}

	return 0;
}

/*
 * Configure a single NAPOT PMA entry.
 * @param pma_region: Region descriptor
 * @param entry_id: PMA entry index to use
 * @return 0 on success, SBI_EINVAL on failure
 */
static int xuantie_pma_setup_napot(const struct xuantie_pma_region *pma_region,
				   unsigned int entry_id)
{
	unsigned long size = pma_region->size;
	unsigned long addr = pma_region->pa;
	unsigned long pmaaddr;
	u8 flags = pma_region->flags;

	/* Validate: must be >= 4KB, naturally aligned power-of-2 */
	if (size < XUANTIE_PMA_GRANULARITY || not_napot(addr, size))
		return SBI_EINVAL;

	/* Ensure NAPOT mode is set in flags */
	flags = (flags & ~XUANTIE_PMACFG_A_MASK) | XUANTIE_PMACFG_A_NAPOT;

	/*
	 * NAPOT encoding (compatible with RISC-V PMP NAPOT):
	 * pmaaddr = (addr >> 2) | ((size >> 3) - 1)
	 * which encodes address[47:2] with trailing 1s for size.
	 */
	pmaaddr = (addr >> 2) | ((size >> 3) - 1);

	/*
	 * Must set cfg (A=NAPOT) BEFORE writing pmaaddr.
	 * When entry is OFF, pmaaddr register may have WARL behavior
	 * that rejects NAPOT-encoded address patterns.
	 */
	set_pma_entry_cfg(entry_id, flags);
	set_pma_entry_addr(entry_id, pmaaddr);

	/* Verify the address write */
	if (get_pma_entry_addr(entry_id) != pmaaddr) {
		sbi_printf("ERROR %s(): Failed to write pmaaddr%d "
			   "(wrote %#lx, read back %#lx)\n",
			   __func__, entry_id, pmaaddr,
			   get_pma_entry_addr(entry_id));
		/* Disable the entry on failure */
		set_pma_entry_cfg(entry_id, XUANTIE_PMACFG_A_OFF);
		return SBI_EINVAL;
	}

	return 0;
}

void xuantie_pma_dump(void)
{
	unsigned long pmaaddr, prev_addr = 0;
	unsigned long start, end;
	u8 cfg, a_mode;

	sbi_printf("PMA entries dump:\n");
	sbi_printf("  %-6s %-6s %-20s %-20s %s\n",
		   "Entry", "Mode", "Start", "End", "Attr");

	for (unsigned int i = 0; i < XUANTIE_MAX_PMA_ENTRIES; i++) {
		cfg = get_pma_entry_cfg(i);
		a_mode = (cfg & XUANTIE_PMACFG_A_MASK) >> XUANTIE_PMACFG_A_SHIFT;
		pmaaddr = get_pma_entry_addr(i);

		switch (a_mode) {
		case 0: /* OFF */
			sbi_printf("  PMA%-2d  OFF    --\n", i);
			prev_addr = pmaaddr << 2;
			break;
		case 1: /* TOR */
			start = prev_addr;
			end = pmaaddr << 2;
			sbi_printf("  PMA%-2d  TOR    %#-20lx %#-20lx %s%s\n",
				   i, start, end,
				   (cfg & XUANTIE_PMACFG_SO) ? "SO" : "WO",
				   (cfg & XUANTIE_PMACFG_CA) ? "+CA" : "+NC");
			prev_addr = end;
			break;
		case 3: { /* NAPOT */
			unsigned long size;

			decode_napot_entry(i, &start, &size);
			end = start + size;
			sbi_printf("  PMA%-2d  NAPOT  %#-20lx %#-20lx %s%s\n",
				   i, start, end,
				   (cfg & XUANTIE_PMACFG_SO) ? "SO" : "WO",
				   (cfg & XUANTIE_PMACFG_CA) ? "+CA" : "+NC");
			prev_addr = pmaaddr << 2;
			break;
		}
		default:
			sbi_printf("  PMA%-2d  RSVD   (A=%d)\n", i, a_mode);
			prev_addr = pmaaddr << 2;
			break;
		}
	}
}

int xuantie_pma_setup_regions(void *fdt,
			     const struct xuantie_pma_region *pma_regions,
			     unsigned int pma_regions_count)
{
	unsigned int i;
	int entry_id;
	int ret;
	u8 a_mode;

	if (!pma_regions || !pma_regions_count)
		return 0;

	if (pma_regions_count > XUANTIE_MAX_PMA_ENTRIES)
		return SBI_EINVAL;

	for (i = 0; i < pma_regions_count; i++) {
		entry_id = i;

		a_mode = pma_regions[i].flags & XUANTIE_PMACFG_A_MASK;

		if (a_mode == XUANTIE_PMACFG_A_TOR) {
			ret = xuantie_pma_setup_tor(&pma_regions[i], entry_id);
		} else if (a_mode == XUANTIE_PMACFG_A_NAPOT) {
			if (has_pma_napot_overlap(pma_regions[i].pa,
						  pma_regions[i].size))
				return SBI_EINVAL;
			ret = xuantie_pma_setup_napot(&pma_regions[i], entry_id);
		} else {
			sbi_printf("ERROR %s(): Unsupported A mode %#x\n",
				   __func__, a_mode);
			return SBI_EINVAL;
		}

		if (ret) {
			sbi_printf("ERROR %s(): Failed to setup PMA entry %d "
				   "for region %#lx size %#lx\n",
				   __func__, entry_id,
				   pma_regions[i].pa, pma_regions[i].size);
			return ret;
		}

		sbi_dprintf("PMA: entry%d %s [%#lx, %#lx) flags=%#x\n",
			    entry_id,
			    (a_mode == XUANTIE_PMACFG_A_TOR) ? "TOR" : "NAPOT",
			    pma_regions[i].pa,
			    pma_regions[i].pa + pma_regions[i].size,
			    pma_regions[i].flags);
	}

	/*
	 * Commit all PMA writes with a single barrier + TLB flush. Doing
	 * this once at the end (rather than per-entry) avoids exposing an
	 * intermediate state where entry i's addr has changed entry i+1's
	 * implicit TOR base while entry i+1 has not been updated yet.
	 */
	RISCV_FENCE(iorw, iorw);
	__asm__ __volatile__("sfence.vma" ::: "memory");

	return 0;
}

int xuantie_pma_set_flags(unsigned int entry_id, u8 flags)
{
	u8 cfg;

	if (entry_id >= XUANTIE_MAX_PMA_ENTRIES)
		return SBI_EINVAL;

	cfg = get_pma_entry_cfg(entry_id);

	/* Entry must be active (not OFF) */
	if (is_pma_entry_disabled(cfg)) {
		sbi_printf("ERROR %s(): PMA entry %d is disabled\n",
			   __func__, entry_id);
		return SBI_EINVAL;
	}

	/* Preserve A mode and reserved bits, update only SO/CA */
	cfg = (cfg & ~(XUANTIE_PMACFG_SO | XUANTIE_PMACFG_CA)) |
	      (flags & (XUANTIE_PMACFG_SO | XUANTIE_PMACFG_CA));

	set_pma_entry_cfg(entry_id, cfg);

	/* Memory barrier and TLB flush */
	RISCV_FENCE(iorw, iorw);
	__asm__ __volatile__("sfence.vma" ::: "memory");

	sbi_dprintf("PMA: entry%d flags updated to %s%s\n",
		   entry_id,
		   (cfg & XUANTIE_PMACFG_SO) ? "SO" : "WO",
		   (cfg & XUANTIE_PMACFG_CA) ? "+CA" : "+NC");

	return 0;
}
