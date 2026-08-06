/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * xuantie Smrnmi (Resumable NMI) support framework.
 *
 * The Smrnmi core infrastructure (asm entry _trap_rnmi_handler and the C
 * dispatcher sbi_trap_rnmi_handler) lives in the common OpenSBI code. This
 * module provides the two xuantie platform callbacks:
 *
 *   - xuantie_smrnmi_handlers_init(): program the hardware NMI vector so the
 *     CPU jumps to the RNMI/RNME asm entry points.
 *   - xuantie_rnmi_handler(): handle the NMI when it fires.
 */

#include <platform_override.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_trap.h>

#include <xuantie/xuantie_rnmi.h>

/*
 * xuantie RNMI vectoring is driven by a single indirectly-accessed
 * rnmi_addr_base register:
 *   - selected through the CSR indirect window (miselect/mireg) with the
 *     vendor selector below;
 *   - 4K-aligned, low 12 bits hardwired to 0;
 *   - NMI         is vectored to rnmi_addr_base;
 *   - double trap is vectored to rnmi_addr_base + 2K.
 */
#define XUANTIE_ISEL_RNMI_ADDR_BASE	_UL(0x8000000000000E01)
#define XUANTIE_RNMI_ADDR_ALIGN_MASK	(~0xFFFUL)
#define XUANTIE_RNMI_DTRAP_OFFSET	_UL(0x800)

void xuantie_smrnmi_handlers_init(void (*rnmi_handler)(void),
				  void (*rnme_handler)(void))
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();
	unsigned long base = (unsigned long)rnmi_handler;

	if (!sbi_hart_has_extension(scratch, SBI_HART_EXT_SMCSRIND) ||
	    !sbi_hart_has_extension(scratch, SBI_HART_EXT_SMRNMI))
		return;

	/*
	 * Validate the handler layout for debugging. The hardware vectors NMI
	 * to rnmi_addr_base and double-trap to rnmi_addr_base + 2K, so the
	 * RNMI entry must be 4K-aligned and the double-trap (RNME) entry must
	 * sit exactly 2K above it.
	 */
	if (base & ~XUANTIE_RNMI_ADDR_ALIGN_MASK)
		sbi_printf("xuantie: RNMI handler %p not 4K-aligned\n",
			   rnmi_handler);
	if ((unsigned long)rnme_handler !=
	    (base & XUANTIE_RNMI_ADDR_ALIGN_MASK) + XUANTIE_RNMI_DTRAP_OFFSET)
		sbi_printf("xuantie: RNME handler %p != RNMI base + 2K\n",
			   rnme_handler);

	/*
	 * TODO: program the xuantie rnmi_addr_base register.
	 *
	 * The required 4K-aligned asm layout (RNMI entry at base, double-trap
	 * at base + 2K) is not in place yet. Once it is, program the base:
	 *
	 *   csr_write(CSR_MISELECT, XUANTIE_ISEL_RNMI_ADDR_BASE);
	 *   csr_write(CSR_MIREG, base & XUANTIE_RNMI_ADDR_ALIGN_MASK);
	 */
}

int xuantie_rnmi_handler(struct sbi_trap_context *tcntx)
{
	/*
	 * TODO: Handle the xuantie NMI source.
	 *
	 * The NMI CSR values are available in the trap context:
	 *   tcntx->trap.cause  = MNCAUSE  (NMI cause)
	 *   tcntx->regs.mepc   = MNEPC    (PC interrupted by the NMI)
	 *   tcntx->regs.mstatus = MNSTATUS (MNPP/MNPV/NMIE of interrupted ctx)
	 *
	 * Typical steps:
	 *   1. Read the NMI source / cause from the xuantie hardware.
	 *   2. Acknowledge / clear the NMI status so it does not re-fire.
	 *   3. Log or take the required recovery action.
	 */
	sbi_printf("xuantie: RNMI cause=0x%lx mepc=0x%lx\n",
		   tcntx->trap.cause, tcntx->regs.mepc);

	return 0;
}
