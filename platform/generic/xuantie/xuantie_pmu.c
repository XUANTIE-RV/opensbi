/*
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_pmu.h>
#include <thead/c9xx_encoding.h>
#include <xuantie/xuantie_pmu.h>

/*
 * Sscofpmf does not provide OF interrupts for the fixed cycle/instret
 * counters. XuanTie cores back those counters with vendor mhpmevent0/2
 * CSRs whose top bits follow the same OF/MINH/...INH layout as the
 * standard mhpmevent3+. The generic sbi_pmu_device hooks let us layer
 * the vendor OF handling on top of the regular Sscofpmf path: the core
 * already calls hw_counter_enable_irq/disable_irq with ctr_idx 0 and 2
 * via the fixed-counter fallback in pmu_fixed_ctr_update_inhibit_bits().
 *
 * Only ctr_idx == 0 (cycle) and ctr_idx == 2 (instret) need vendor
 * handling. Other counters are programmable mhpmevent3+ and follow the
 * standard Sscofpmf code path unchanged.
 */

static void xuantie_pmu_ctr_enable_irq(uint32_t ctr_idx)
{
	if (ctr_idx != 0 && ctr_idx != 2)
		return;

	/*
	 * Mirror pmu_ctr_enable_irq_hw(): only clear OF when no lcofip is
	 * still pending, so we don't race with software that hasn't yet
	 * handled the previous overflow.
	 */
	if (csr_read(CSR_MIP) & MIP_LCOFIP)
		return;

#if __riscv_xlen == 32
	/* OF lives in the H half on RV32; clear via mhpmevent0H/mhpmevent2H. */
	if (ctr_idx == 0)
		csr_clear(CSR_MHPMEVENT0H, MHPMEVENTH_OF);
	else
		csr_clear(CSR_MHPMEVENT2H, MHPMEVENTH_OF);
#else
	if (ctr_idx == 0)
		csr_clear(CSR_MHPMEVENT0, MHPMEVENT_OF);
	else
		csr_clear(CSR_MHPMEVENT2, MHPMEVENT_OF);
#endif
}

static void xuantie_pmu_ctr_disable_irq(uint32_t ctr_idx)
{
	if (ctr_idx != 0 && ctr_idx != 2)
		return;

	/*
	 * Setting OF latches the counter so a subsequent overflow cannot
	 * raise lcofip until enable_irq clears it again. This matches the
	 * "OF set = disabled" convention used by pmu_update_hw_mhpmevent().
	 */
#if __riscv_xlen == 32
	if (ctr_idx == 0)
		csr_set(CSR_MHPMEVENT0H, MHPMEVENTH_OF);
	else
		csr_set(CSR_MHPMEVENT2H, MHPMEVENTH_OF);
#else
	if (ctr_idx == 0)
		csr_set(CSR_MHPMEVENT0, MHPMEVENT_OF);
	else
		csr_set(CSR_MHPMEVENT2, MHPMEVENT_OF);
#endif
}

static const struct sbi_pmu_device xuantie_pmu_device = {
	.name			= "xuantie,pmu",
	.hw_counter_enable_irq	= xuantie_pmu_ctr_enable_irq,
	.hw_counter_disable_irq	= xuantie_pmu_ctr_disable_irq,
};

void xuantie_pmu_register_device(void)
{
	sbi_pmu_set_device(&xuantie_pmu_device);
}

void xuantie_pmu_enable_ofint(void)
{
	csr_set(THEAD_C9XX_CSR_MXSTATUS, MXSTATUS_OFINT);
}
