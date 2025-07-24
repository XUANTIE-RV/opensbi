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

static u32 gquirk = 0;

int xuantie_final_init(bool cold_boot)
{
	if (cold_boot) {
		if (gquirk & QUIRK_XUANTIE_PMC)
			xuantie_pmc_device_init();
		if (gquirk & QUIRK_XUANTIE_LINK)
			xuantie_link_pmu_device_init();
	}

	return generic_final_init(cold_boot);
}

static int xuantie_dummy_platform_init(const void *fdt, int nodeoff,
				       const struct fdt_match *match)
{
	const struct xuantie_generic_quirks *data = match->data;

	gquirk = data->quirk;
	generic_platform_ops.final_init = xuantie_final_init;

	return 0;
}

static const struct xuantie_generic_quirks xuantie_quirks = {
	.quirk = QUIRK_XUANTIE_PMC | QUIRK_XUANTIE_LINK,
};

static const struct xuantie_generic_quirks xuantie_pmc_quirks = {
	.quirk = QUIRK_XUANTIE_PMC,
};

static const struct xuantie_generic_quirks xuantie_link_quirks = {
	.quirk = QUIRK_XUANTIE_LINK,
};

static const struct fdt_match xuantie_dummy_match[] = {
	{ .compatible = "xuantie,dummy", .data = &xuantie_quirks },
	{ .compatible = "xuantie,pmc", .data = &xuantie_pmc_quirks },
	{ .compatible = "xuantie,link", .data = &xuantie_link_quirks },
	{ },
};

const struct fdt_driver xuantie_dummy = {
	.match_table = xuantie_dummy_match,
	.init = xuantie_dummy_platform_init,
};