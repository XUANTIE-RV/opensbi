/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2026 Alibaba Group Holding Limited.
 */

#ifndef __SERIAL_PL011_UART_H__
#define __SERIAL_PL011_UART_H__

#include <sbi/sbi_types.h>

int pl011_uart_init(unsigned long base, u32 in_freq, u32 baudrate);

#endif
