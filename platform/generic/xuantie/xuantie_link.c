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
	int nodeoffset, rc, dev_type;
	uint64_t addr;
	void *fdt = fdt_get_address_rw();

	if ((nodeoffset = fdt_node_offset_by_compatible(fdt, -1, "xuantie,xl100-pmu")) > 0)
		dev_type = XUANTIE_LINK_DEV_TYPE_XL100;
	else if ((nodeoffset = fdt_node_offset_by_compatible(fdt, -1, "xuantie,xl200-pmu")) > 0)
		dev_type = XUANTIE_LINK_DEV_TYPE_XL200;
	else if ((nodeoffset = fdt_node_offset_by_compatible(fdt, -1, "xuantie,xl300-pmu")) > 0)
		dev_type = XUANTIE_LINK_DEV_TYPE_XL300;
	else
		return SBI_ENODEV;

	rc = fdt_get_node_addr_size(fdt, nodeoffset, 0, &addr, NULL);
	if (rc)
		return SBI_ENODEV;

	if (dev_type == XUANTIE_LINK_DEV_TYPE_XL300)
		writeq(BIT_ULL(40) | BIT_ULL(32) | 0x3f, (void *)addr + XUANTIE_LINK_PMU_HPCPHAUTHCR);
	writeq(0x3f, (void *)addr + XUANTIE_LINK_PMU_HPCPMAUTHCR);
	writeq(0x3f, (void *)addr + XUANTIE_LINK_PMU_HPCPSAUTHCR);
	writeq(0x1, (void *)addr + XUANTIE_LINK_PMU_L3MAUTHCR);
	writeq(0x0, (void *)addr + XUANTIE_LINK_PMU_HPCPINHIBIT);
#endif
	return SBI_OK;
}
