/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2026 Alibaba Group Holding Limited.
 */

#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/serial/fdt_serial.h>
#include <sbi_utils/serial/pl011.h>

static int serial_pl011_init(const void *fdt, int nodeoff,
			     const struct fdt_match *match)
{
	int rc;
	struct platform_uart_data uart = { 0 };

	rc = fdt_parse_uart_node(fdt, nodeoff, &uart);
	if (rc)
		return rc;

	return pl011_uart_init(uart.addr, uart.freq, uart.baud);
}

static const struct fdt_match serial_pl011_match[] = {
	{ .compatible = "arm,pl011" },
	{ .compatible = "arm,sbsa-uart" },
	{ },
};

const struct fdt_driver fdt_serial_pl011 = {
	.match_table = serial_pl011_match,
	.init = serial_pl011_init
};
