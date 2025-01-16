#
# SPDX-License-Identifier: BSD-2-Clause
#

carray-platform_override_modules-$(CONFIG_PLATFORM_XUANTIE) += xuantie_pmc
platform-objs-$(CONFIG_PLATFORM_XUANTIE) += xuantie/xuantie_pmc.o
