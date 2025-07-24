#
# SPDX-License-Identifier: BSD-2-Clause
#

carray-platform_override_modules-$(CONFIG_PLATFORM_XUANTIE) += xuantie_dummy
platform-objs-$(CONFIG_PLATFORM_XUANTIE) += xuantie/xuantie_dummy.o xuantie/xuantie_pmc.o xuantie/xuantie_link.o
