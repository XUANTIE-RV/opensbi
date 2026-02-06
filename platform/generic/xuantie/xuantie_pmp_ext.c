/*
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libfdt.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_console.h>
#include <sbi/riscv_io.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <xuantie/xuantie_pmp_ext.h>

/*
sbi_domain_root_add_memrange needs to be called before sbi_domain_finalize,
because sbi_domain_finalize sets domain_finalized = true, causing subsequent
additions to fail.
*/
int xuantie_pmp_ext_cfg(void)
{
	int nodeoffset, rc;
	uint64_t addr, size;
	void *fdt = fdt_get_address_rw();

	nodeoffset = fdt_node_offset_by_compatible(fdt, -1, "xuantie,pmp_ext");
	if (nodeoffset < 0)
		return nodeoffset;

	rc = fdt_get_node_addr_size(fdt, nodeoffset, 0, &addr, &size);
	if (rc)
		return SBI_ENODEV;

	rc = sbi_domain_root_add_memrange(addr, size, size & (-size),
						  SBI_DOMAIN_MEMREGION_ENF_PERMISSIONS);
	return rc;
}
