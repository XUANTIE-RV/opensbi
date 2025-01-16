/*
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef __RISCV_XUANTIE_PMC_H__
#define __RISCV_XUANTIE_PMC_H__

#include <sbi/sbi_types.h>

/* clang-format off */

#define XUANTIE_DCACHE_INVAL_CLEAN_ALL	".long 0x0030000b"
#define XUANTIE_SYNC_S					".long 0x0190000b"

/*
 core    PMC: blk1 = VFPU; blk2 = MPU
 cluster PMC: blk1 = l2cache
 */
#define PMC_BLK_CORE		0
#define PMC_BLK_CORE_VFPU	1
#define PMC_BLK_CORE_MPU	2
#define PMC_BLK_CLUSTER		0
#define PMC_BLK_CLUSTER_L2	1

#define PMC_TRANS_MODE_SHIFT		4
#define PMC_TRANS_MODE_DIRECT		(_UL(1) << PMC_TRANS_MODE_SHIFT)
#define PMC_TRANS_MODE_DYNAMIC		(_UL(0) << PMC_TRANS_MODE_SHIFT)
/*pmc_blk_mode_config (basic type)*/
#define PMC_BLK_MODE_CONFIG_0		0 // OFF pstate enable
#define PMC_BLK_MODE_CONFIG_1		1
#define PMC_BLK_MODE_CONFIG_2		2
#define PMC_BLK_MODE_CONFIG_3		3
#define PMC_BLK_MODE_CONFIG_4		4
#define PMC_BLK_MODE_CONFIG_5		5
#define PMC_BLK_MODE_CONFIG_6		6
#define PMC_BLK_MODE_CONFIG_7		7
#define PMC_BLK_MODE_CONFIG_8		8 // ON pstate enable
#define PMC_BLK_MODE_CONFIG_9		9
#define PMC_BLK_MODE_CONFIG_ON		PMC_BLK_MODE_CONFIG_8
#define PMC_BLK_MODE_CONFIG_OFF		PMC_BLK_MODE_CONFIG_0
/*pmc_blk_mode_config (pe type)*/
#define PMC_BLK_LOW_POWER_STATE_OFF 0
#define PMC_BLK_LOW_POWER_STATE_RET 1


/* clang-format on */

struct xuantie_pmc_data {
	unsigned long pmc_addr;
};

int xuantie_pmc_device_init(void);

#endif /* __RISCV_XUANTIE_PMC_H__ */
