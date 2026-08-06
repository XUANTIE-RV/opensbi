/*
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef __RISCV_XUANTIE_SMRNMI_H__
#define __RISCV_XUANTIE_SMRNMI_H__

struct sbi_trap_context;

/*
 * Install the RNMI/RNME assembly entry points into the xuantie hardware.
 *
 * @rnmi_handler:  asm entry for resumable NMI (_trap_rnmi_handler)
 * @rnme_handler:  asm entry for RNME, taken as a regular M-mode trap
 *                 with NMIE=0 (_trap_handler)
 *
 * Called from sbi_hart.c when Smrnmi is detected. The way the NMI vector
 * address is programmed is implementation defined by the xuantie hardware.
 */
void xuantie_smrnmi_handlers_init(void (*rnmi_handler)(void),
				  void (*rnme_handler)(void));

/*
 * C handler invoked when an RNMI fires. The trap context carries the NMI
 * CSR values: tcntx->trap.cause = MNCAUSE, tcntx->regs.mepc = MNEPC,
 * tcntx->regs.mstatus = MNSTATUS.
 *
 * Returns 0 on success, or a negative SBI error code if the NMI could not
 * be handled.
 */
int xuantie_rnmi_handler(struct sbi_trap_context *tcntx);

#endif /* __RISCV_XUANTIE_SMRNMI_H__ */
