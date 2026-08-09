/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2026 Alibaba Group Holding Limited.
 */

#ifndef _RISCV_XUANTIE_PMA_H_
#define _RISCV_XUANTIE_PMA_H_

#include <sbi/sbi_types.h>

/* XuanTie PMA uses indirect CSR access via miselect/mireg */
#define XUANTIE_PMA_MISELECT_BASE	0x8000000000000000UL
#define XUANTIE_PMA_CSR_PMACFG0		(XUANTIE_PMA_MISELECT_BASE | 0x00)
#define XUANTIE_PMA_CSR_PMACFG2		(XUANTIE_PMA_MISELECT_BASE | 0x02)
#define XUANTIE_PMA_CSR_PMAADDR0	(XUANTIE_PMA_MISELECT_BASE | 0x10)

/* Maximum PMA entries */
#define XUANTIE_MAX_PMA_ENTRIES		16

/* Minimum granularity for NAPOT mode: 4KB */
#define XUANTIE_PMA_GRANULARITY		(1UL << 12)

/*
 * PMA Configuration Bitfield (per entry, 8 bits):
 *   [7:5] Reserved
 *   [4:3] A - Address matching mode
 *   [2]   Reserved
 *   [1]   SO - Strongly Ordered (1) / Weakly Ordered (0)
 *   [0]   CA - Cacheable (1) / Non-cacheable (0)
 */
#define XUANTIE_PMACFG_A_SHIFT		3
#define XUANTIE_PMACFG_A_MASK		(0x3UL << XUANTIE_PMACFG_A_SHIFT)
#define XUANTIE_PMACFG_A_OFF		(0x0UL << XUANTIE_PMACFG_A_SHIFT)
#define XUANTIE_PMACFG_A_TOR		(0x1UL << XUANTIE_PMACFG_A_SHIFT)
#define XUANTIE_PMACFG_A_NAPOT		(0x3UL << XUANTIE_PMACFG_A_SHIFT)

#define XUANTIE_PMACFG_SO_SHIFT		1
#define XUANTIE_PMACFG_SO		(1UL << XUANTIE_PMACFG_SO_SHIFT)
#define XUANTIE_PMACFG_WO		(0UL << XUANTIE_PMACFG_SO_SHIFT)

#define XUANTIE_PMACFG_CA_SHIFT		0
#define XUANTIE_PMACFG_CA		(1UL << XUANTIE_PMACFG_CA_SHIFT)
#define XUANTIE_PMACFG_NC		(0UL << XUANTIE_PMACFG_CA_SHIFT)

/*
 * struct xuantie_pma_region - Describes a XuanTie PMA region
 *
 * @pa: Physical address of the region start
 * @size: Size of the region (must be power-of-2 and >= 4KB for NAPOT)
 * @flags: PMA configuration flags (A mode | SO | CA)
 */
struct xuantie_pma_region {
	unsigned long pa;
	unsigned long size;
	u8 flags;
};

/*
 * Configure PMA regions and optionally fixup device tree
 *
 * @param fdt: Pointer to FDT (can be NULL to skip DT fixup)
 * @param pma_regions: Array of PMA region descriptors
 * @param pma_regions_count: Number of regions
 * @return 0 on success, negative error code on failure
 */
int xuantie_pma_setup_regions(void *fdt,
			     const struct xuantie_pma_region *pma_regions,
			     unsigned int pma_regions_count);

/*
 * Dump all PMA entries status (mode, address range, attributes)
 */
void xuantie_pma_dump(void);

/*
 * Modify the memory attributes (SO/CA) of an existing PMA entry.
 * Preserves the entry's A mode and address, only changes attribute bits.
 *
 * @param entry_id: PMA entry index (0-15)
 * @param flags: New attribute flags (XUANTIE_PMACFG_SO | XUANTIE_PMACFG_CA)
 * @return 0 on success, negative error code on failure
 */
int xuantie_pma_set_flags(unsigned int entry_id, u8 flags);

#endif /* _RISCV_XUANTIE_PMA_H_ */
