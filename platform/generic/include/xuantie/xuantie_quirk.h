/*
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef __RISCV_XUANTIE_QUIRK_H__
#define __RISCV_XUANTIE_QUIRK_H__

#define QUIRK_XUANTIE_PMC BIT(0)

struct xuantie_generic_quirks {
	u32 quirk;
};

#endif // __RISCV_XUANTIE_QUIRK_H__
