#
# SPDX-License-Identifier: BSD-2-Clause
#

carray-platform_override_modules-$(CONFIG_PLATFORM_XUANTIE) += xuantie_ipmc_test
platform-objs-$(CONFIG_PLATFORM_XUANTIE) += xuantie/xuantie_ipmc_test.o
