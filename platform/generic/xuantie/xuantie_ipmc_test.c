/*
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <platform_override.h>
#include <thead/c9xx_encoding.h>
#include <thead/c9xx_pmu.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_system.h>
#include <sbi/sbi_pmu.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_platform.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/irqchip/fdt_irqchip_plic.h>

#include "xuantie_ipmc_test.h"

static struct xuantie_ipmc_data ipmc = { 0 };
static unsigned long zsb_addr = 0;

static inline void xuantie_riscv_cfg_init(void)
{
	struct sbi_scratch *scratch = sbi_scratch_thishart_ptr();
	if (!zsb_addr) {
		zsb_addr = scratch->next_addr - 0x00008000;
	}
}

static inline int ipmc_set_command(struct xuantie_ipmc_data *ipmc, u32 hartid, u32 blkid, u32 val)
{
	unsigned long ipmc_base, blk_sw_mode_ctl;
	if (ipmc) {
		ipmc_base = ipmc->ipmc_addr + 0x1000 * hartid;
		blk_sw_mode_ctl = 0x100 + 0x4 * blkid;
		writew_relaxed(val, (void *)(ipmc_base + blk_sw_mode_ctl));
		return 0;
	} else
		return SBI_EINVAL;
}

static inline void xuantie_hart_down(u32 hartid, bool wakeup)
{
	if (wakeup) {
		/* Cpuidle power down, set trans bit to auto mode */
		ipmc_set_command(&ipmc, hartid, IPMC_BLK_CORE, IPMC_TRANS_MODE_DYNAMIC | IPMC_BLK_MODE_CONFIG_OFF);
		ipmc_set_command(&ipmc, hartid, IPMC_BLK_CORE_VFPU, IPMC_TRANS_MODE_DYNAMIC | IPMC_BLK_LOW_POWER_STATE_OFF);
	} else {
		/* Hotplug power down, set trans bit to manual mode */
		ipmc_set_command(&ipmc, hartid, IPMC_BLK_CORE, IPMC_TRANS_MODE_DYNAMIC | IPMC_BLK_MODE_CONFIG_OFF);
		ipmc_set_command(&ipmc, hartid, IPMC_BLK_CORE_VFPU, IPMC_TRANS_MODE_DYNAMIC | IPMC_BLK_LOW_POWER_STATE_OFF);
	}

	/* Close the Prefetch */
	csr_clear(THEAD_C9XX_CSR_MHINT, 0x1 << 2);

	/* Clean and invalidate core's dcache */
	asm volatile(XUANTIE_DCACHE_INVAL_CLEAN_ALL);
	asm volatile(XUANTIE_SYNC_IS);

	/* Close the Dcache */
	csr_clear(THEAD_C9XX_CSR_MHCR, 0x1 << 1);

	/* Close the snoop */
	csr_clear(THEAD_C9XX_CSR_MSMPR, 0x1);

	csr_set(CSR_MIE, MIP_MTIP | MIP_STIP | MIP_SEIP | MIP_MEIP);

	wfi();

	__asm__ __volatile__(
		"jalr x0, %0\n\t"
		:
		: "r"(zsb_addr)
		: "memory");
}

static int xuantie_hart_suspend(u32 suspend_type)
{
	u32 hartid = current_hartid();

	/* Use the generic code for retentive suspend. */
	if (!(suspend_type & SBI_HSM_SUSP_NON_RET_BIT))
		return SBI_ENOTSUPP;

	xuantie_hart_down(hartid, true);

	return 0;
}

static void xuantie_hart_resume(void)
{
}

int xuantie_hart_start(u32 hartid, ulong saddr)
{
	return 0;
}

int xuantie_hart_stop(void)
{
	u32 hartid = current_hartid();

	xuantie_hart_down(hartid, false);

	return 0;
}

static const struct sbi_hsm_device xuantie_hsm = {
	.name			= "xuantie-hsm",
	.hart_suspend	= xuantie_hart_suspend,
	.hart_resume	= xuantie_hart_resume,
	.hart_start 	= xuantie_hart_start,
	.hart_stop		= xuantie_hart_stop,
};

static int xuantie_system_suspend_check(u32 sleep_type)
{
	return sleep_type == SBI_SUSP_SLEEP_TYPE_SUSPEND ? 0 : SBI_EINVAL;
}

static int xuantie_system_suspend(u32 sleep_type, unsigned long mmode_resume_addr)
{
	u32 hartid = current_hartid();

	if (sleep_type != SBI_SUSP_SLEEP_TYPE_SUSPEND)
		return SBI_EINVAL;

	/* enable cpuidle, cpu poweroff after wfi */
	xuantie_hart_down(hartid, true);

	return 0;
}

static struct sbi_system_suspend_device xuantie_system_suspend_dev = {
	.name = "xuantie-system-suspend",
	.system_suspend_check = xuantie_system_suspend_check,
	.system_suspend = xuantie_system_suspend,
};

static int xuantie_system_reset_check(u32 type, u32 reason)
{
	switch (type) {
	case SBI_SRST_RESET_TYPE_SHUTDOWN:
		return 1;
	case SBI_SRST_RESET_TYPE_COLD_REBOOT:
	case SBI_SRST_RESET_TYPE_WARM_REBOOT:
		return 255;
	}

	return 0;
}

static void xuantie_system_reset(u32 type, u32 reason)
{
}

static struct sbi_system_reset_device xuantie_reset = {
	.name = "xuantie-reset",
	.system_reset_check = xuantie_system_reset_check,
	.system_reset = xuantie_system_reset
};

static int xuantie_ipmc_test_device_init(void)
{
	void *fdt = fdt_get_address();
	int rc = fdt_parse_compat_addr(fdt, (uint64_t *)&ipmc.ipmc_addr, "xuantie,ipmc");
	if (rc) {
		return rc;
	}

	sbi_system_reset_add_device(&xuantie_reset);
	sbi_hsm_set_device(&xuantie_hsm);
	sbi_system_suspend_set_device(&xuantie_system_suspend_dev);

	return 0;
}

static int xuantie_final_init(bool cold_boot, const struct fdt_match *match)
{
	if (cold_boot) {
		xuantie_riscv_cfg_init();
		xuantie_ipmc_test_device_init();
	}

	return 0;
}

static const struct fdt_match xuantie_ipmc_test_match[] = {
	{ .compatible = "xuantie,lowpower" },
	{ },
};

const struct platform_override xuantie_ipmc_test = {
	.match_table	 = xuantie_ipmc_test_match,
	.final_init	 = xuantie_final_init,
};
