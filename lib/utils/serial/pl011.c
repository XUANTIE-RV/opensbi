/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2026 Alibaba Group Holding Limited.
 *
 * ARM PrimeCell PL011 UART driver.
 */

#include <sbi/sbi_domain.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_console.h>
#include <sbi_utils/serial/pl011.h>

/* clang-format off */

#define UART_REG_DR		0x00	/* Data register */
#define UART_REG_FR		0x18	/* Flag register */
#define UART_REG_IBRD		0x24	/* Integer baud rate divisor */
#define UART_REG_FBRD		0x28	/* Fractional baud rate divisor */
#define UART_REG_LCR_H		0x2c	/* Line control register */
#define UART_REG_CR		0x30	/* Control register */
#define UART_REG_IMSC		0x38	/* Interrupt mask set/clear register */

#define UART_FR_RXFE		(1 << 4)	/* Receive FIFO empty */
#define UART_FR_TXFF		(1 << 5)	/* Transmit FIFO full */

#define UART_LCR_H_FEN		(1 << 4)	/* Enable FIFOs */
#define UART_LCR_H_WLEN_8	(3 << 5)	/* 8 bits word length */

#define UART_CR_UARTEN		(1 << 0)	/* UART enable */
#define UART_CR_TXE		(1 << 8)	/* Transmit enable */
#define UART_CR_RXE		(1 << 9)	/* Receive enable */

/* clang-format on */

static volatile void *uart_base;
static u32 uart_in_freq;
static u32 uart_baudrate;

static u32 get_reg(u32 offset)
{
	return readl(uart_base + offset);
}

static void set_reg(u32 offset, u32 val)
{
	writel(val, uart_base + offset);
}

static void pl011_uart_putc(char ch)
{
	while (get_reg(UART_REG_FR) & UART_FR_TXFF)
		;

	set_reg(UART_REG_DR, ch);
}

static int pl011_uart_getc(void)
{
	if (!(get_reg(UART_REG_FR) & UART_FR_RXFE))
		return get_reg(UART_REG_DR);

	return -1;
}

static struct sbi_console_device pl011_console = {
	.name = "pl011",
	.console_putc = pl011_uart_putc,
	.console_getc = pl011_uart_getc
};

int pl011_uart_init(unsigned long base, u32 in_freq, u32 baudrate)
{
	uart_base     = (volatile void *)base;
	uart_in_freq  = in_freq;
	uart_baudrate = baudrate;

	/* Disable the UART before configuration */
	set_reg(UART_REG_CR, 0);

	/* Mask all interrupts */
	set_reg(UART_REG_IMSC, 0);

	/*
	 * Program the baud rate divisors only when both the input clock
	 * frequency and the target baud rate are known. On platforms (such
	 * as QEMU) that do not expose "clock-frequency", in_freq is 0 and
	 * the existing divisor configuration is left untouched.
	 */
	if (in_freq && baudrate) {
		u32 div = (in_freq * 4) / baudrate;

		set_reg(UART_REG_IBRD, div >> 6);
		set_reg(UART_REG_FBRD, div & 0x3f);
	}

	/* 8 bits word length, enable FIFOs, no parity, 1 stop bit */
	set_reg(UART_REG_LCR_H, UART_LCR_H_WLEN_8 | UART_LCR_H_FEN);

	/* Enable the UART with both transmit and receive paths */
	set_reg(UART_REG_CR, UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE);

	sbi_console_set_device(&pl011_console);

	return sbi_domain_root_add_memrange(base, PAGE_SIZE, PAGE_SIZE,
					    (SBI_DOMAIN_MEMREGION_MMIO |
					     SBI_DOMAIN_MEMREGION_SHARED_SURW_MRW));
}
