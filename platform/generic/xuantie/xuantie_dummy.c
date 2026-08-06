/*
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <platform_override.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_string.h>
#include <sbi/sbi_console.h>
#include <sbi_utils/fdt/fdt_helper.h>

#include <xuantie/xuantie_quirk.h>
#include <xuantie/xuantie_pmc.h>
#include <xuantie/xuantie_link.h>
#include <xuantie/xuantie_pmp_ext.h>
#include <xuantie/xuantie_pmu.h>
#include <xuantie/xuantie_rnmi.h>

static u32 gquirk = 0;

int xuantie_early_init(bool cold_boot)
{
	if (cold_boot) {
		if (gquirk & QUIRK_XUANTIE_PMP_EXT)
			xuantie_pmp_ext_cfg();
	}

	/* mxstatus[8] OFINT is per-hart: enable on every boot path. */
	if (gquirk & QUIRK_XUANTIE_PMU)
		xuantie_pmu_enable_ofint();

	return generic_early_init(cold_boot);
}

int xuantie_final_init(bool cold_boot)
{
	if (cold_boot) {
		if (gquirk & QUIRK_XUANTIE_PMC)
			xuantie_pmc_device_init();
		if (gquirk & QUIRK_XUANTIE_LINK)
			xuantie_link_pmu_device_init();
		if (gquirk & QUIRK_XUANTIE_PMU)
			xuantie_pmu_register_device();
	}

	return generic_final_init(cold_boot);
}

static int xuantie_dummy_platform_init(const void *fdt, int nodeoff,
				       const struct fdt_match *match)
{
	const struct xuantie_generic_quirks *data = match->data;

	gquirk = data->quirk;
	generic_platform_ops.early_init = xuantie_early_init;
	generic_platform_ops.final_init = xuantie_final_init;

	if (gquirk & QUIRK_XUANTIE_RNMI) {
		generic_platform_ops.smrnmi_handlers_init = xuantie_smrnmi_handlers_init;
		generic_platform_ops.rnmi_handler = xuantie_rnmi_handler;
	}

	return 0;
}

static const struct xuantie_generic_quirks xuantie_quirks = {
	.quirk = QUIRK_XUANTIE_PMC | QUIRK_XUANTIE_LINK | QUIRK_XUANTIE_PMP_EXT |
		 QUIRK_XUANTIE_PMU | QUIRK_XUANTIE_RNMI,
};

static const struct xuantie_generic_quirks xuantie_pmc_quirks = {
	.quirk = QUIRK_XUANTIE_PMC,
};

static const struct xuantie_generic_quirks xuantie_link_quirks = {
	.quirk = QUIRK_XUANTIE_LINK,
};

static const struct xuantie_generic_quirks xuantie_pmp_ext_quirks = {
	.quirk = QUIRK_XUANTIE_PMP_EXT,
};

static const struct xuantie_generic_quirks xuantie_pmu_quirks = {
	.quirk = QUIRK_XUANTIE_PMU,
};

static const struct xuantie_generic_quirks xuantie_rnmi_quirks = {
	.quirk = QUIRK_XUANTIE_RNMI,
};

static const struct fdt_match xuantie_dummy_match[] = {
	{ .compatible = "xuantie,dummy", .data = &xuantie_quirks },
	{ .compatible = "xuantie,pmc", .data = &xuantie_pmc_quirks },
	{ .compatible = "xuantie,link", .data = &xuantie_link_quirks },
	{ .compatible = "xuantie,pmu", .data = &xuantie_pmu_quirks },
	{ .compatible = "xuantie,rnmi", .data = &xuantie_rnmi_quirks },
	{ .compatible = "riscv-virtio",  .data = &xuantie_pmp_ext_quirks }, // qemu debug
	{ },
};

const struct fdt_driver xuantie_dummy = {
	.match_table = xuantie_dummy_match,
	.init = xuantie_dummy_platform_init,
};