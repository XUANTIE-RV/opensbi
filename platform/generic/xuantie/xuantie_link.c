/*
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libfdt.h>
#include <sbi/sbi_error.h>
#include <sbi/riscv_io.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <xuantie/xuantie_link.h>

int xuantie_link_pmu_device_init(void)
{
#if __riscv_xlen == 64
	static const struct {
		const char *compat;
		int dev_type;
	} pmu_match[] = {
		{ "xuantie,xl100-pmu", XUANTIE_LINK_DEV_TYPE_XL100 },
		{ "xuantie,xl200-pmu", XUANTIE_LINK_DEV_TYPE_XL200 },
		{ "xuantie,xl300-pmu", XUANTIE_LINK_DEV_TYPE_XL300 },
	};
	void *fdt = fdt_get_address_rw();
	bool found = false;
	unsigned int i;

	if (!fdt)
		return SBI_ENODEV;

	for (i = 0; i < array_size(pmu_match); i++) {
		int nodeoffset = -1;

		while ((nodeoffset = fdt_node_offset_by_compatible(
				fdt, nodeoffset, pmu_match[i].compat)) >= 0) {
			uint64_t addr;

			if (!fdt_node_is_enabled(fdt, nodeoffset))
				continue;
			if (fdt_get_node_addr_size(fdt, nodeoffset, 0, &addr, NULL))
				continue;

			if (pmu_match[i].dev_type == XUANTIE_LINK_DEV_TYPE_XL300)
				writeq(XUANTIE_LINK_PMU_AUTHCR_AUTH |
				       XUANTIE_LINK_PMU_AUTHCR_INTPEND |
				       XUANTIE_LINK_PMU_CNT_BITMAP,
				       (void *)addr + XUANTIE_LINK_PMU_HPCPHAUTHCR);
			writeq(XUANTIE_LINK_PMU_CNT_BITMAP, (void *)addr + XUANTIE_LINK_PMU_HPCPMAUTHCR);
			writeq(XUANTIE_LINK_PMU_L3AUTHCR_ALLAU |
			       XUANTIE_LINK_PMU_L3AUTHCR_L3CBQ_CCAU,
			       (void *)addr + XUANTIE_LINK_PMU_L3MAUTHCR);
			writeq(0, (void *)addr + XUANTIE_LINK_PMU_HPCPINHIBIT);
			found = true;
		}
	}

	if (!found)
		return SBI_ENODEV;
#endif
	return SBI_OK;
}
