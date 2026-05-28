/*
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef __RISCV_XUANTIE_PMU_H__
#define __RISCV_XUANTIE_PMU_H__

/*
 * XuanTie cores expose Sscofpmf-style OF semantics on the fixed cycle and
 * instret counters via vendor per-counter event registers (mhpmevent0/2,
 * and the matching high halves on RV32). The CSR addresses live in
 * <sbi/riscv_encoding.h> next to the standard mhpmevent CSRs.
 *
 * mxstatus[8] (OFINT) is the per-hart master gate that lets the vendor
 * OF bit raise lcofip; xuantie_pmu_enable_ofint() sets it.
 */

#define MXSTATUS_OFINT	BIT(8)

void xuantie_pmu_register_device(void);
void xuantie_pmu_enable_ofint(void);

#endif /* __RISCV_XUANTIE_PMU_H__ */
