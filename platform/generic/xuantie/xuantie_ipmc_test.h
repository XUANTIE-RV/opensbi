/*
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __XUANTIE_IPMC_TEST_H__
#define __XUANTIE_IPMC_TEST_H__

#include <sbi/sbi_types.h>

/* clang-format off */

#define XUANTIE_DCACHE_INVAL_CLEAN_ALL	".long 0x0030000b"
#define XUANTIE_SYNC_IS					".long 0x01b0000b"

/*
 core    IPMC: blk1 = VFPU; blk2 = MPU
 cluster IPMC: blk1 = l2cache
 */
#define IPMC_BLK_CORE		0
#define IPMC_BLK_CORE_VFPU	1
#define IPMC_BLK_CORE_MPU	2
#define IPMC_BLK_CLUSTER	0
#define IPMC_BLK_CLUSTER_L2	1

#define IPMC_TRANS_MODE_SHIFT	4
#define IPMC_TRANS_MODE_DIRECT	(_UL(1) << IPMC_TRANS_MODE_SHIFT)
#define IPMC_TRANS_MODE_DYNAMIC	(_UL(0) << IPMC_TRANS_MODE_SHIFT)
/*pmc_blk_mode_config (basic type)*/
#define IPMC_BLK_MODE_CONFIG_0		0 // OFF pstate enable
#define IPMC_BLK_MODE_CONFIG_1		1
#define IPMC_BLK_MODE_CONFIG_2		2
#define IPMC_BLK_MODE_CONFIG_3		3
#define IPMC_BLK_MODE_CONFIG_4		4
#define IPMC_BLK_MODE_CONFIG_5		5
#define IPMC_BLK_MODE_CONFIG_6		6
#define IPMC_BLK_MODE_CONFIG_7		7
#define IPMC_BLK_MODE_CONFIG_8		8 // ON pstate enable
#define IPMC_BLK_MODE_CONFIG_9		9
#define IPMC_BLK_MODE_CONFIG_ON		IPMC_BLK_MODE_CONFIG_8
#define IPMC_BLK_MODE_CONFIG_OFF	IPMC_BLK_MODE_CONFIG_0
/*pmc_blk_mode_config (pe type)*/
#define IPMC_BLK_LOW_POWER_STATE_OFF 0
#define IPMC_BLK_LOW_POWER_STATE_RET 1


/* clang-format on */

struct xuantie_ipmc_data {
	unsigned long ipmc_addr;
};

#endif /* __XUANTIE_IPMC_TEST_H__ */
