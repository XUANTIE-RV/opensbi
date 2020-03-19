/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/riscv_encoding.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_unpriv.h>

#define DEFINE_UNPRIVILEGED_LOAD_FUNCTION(type, insn)                         \
	type sbi_load_##type(const type *addr,                                \
			     struct sbi_scratch *scratch,                     \
			     struct sbi_trap_info *trap)                      \
	{                                                                     \
		register ulong tinfo asm("a3");                               \
		register ulong ttmp asm("a4");                                \
		register ulong mstatus asm("a5");                             \
		register ulong mtvec asm("a6") = sbi_hart_unpriv_trap_addr(); \
		type ret = 0;                                                 \
		trap->cause = 0;                                              \
		asm volatile(                                                 \
			"add %[tinfo], %[taddr], zero\n"                      \
			"add %[ttmp], %[taddr], zero\n"                       \
			"csrrw %[mtvec], " STR(CSR_MTVEC) ", %[mtvec]\n"      \
			"csrrs %[mstatus], " STR(CSR_MSTATUS) ", %[mprv]\n"   \
			".option push\n"                                      \
			".option norvc\n"                                     \
			#insn " %[ret], %[addr]\n"                            \
			".option pop\n"                                       \
			"csrw " STR(CSR_MSTATUS) ", %[mstatus]\n"             \
			"csrw " STR(CSR_MTVEC) ", %[mtvec]"                   \
		    : [mstatus] "+&r"(mstatus), [mtvec] "+&r"(mtvec),         \
		      [tinfo] "+&r"(tinfo), [ttmp] "+&r"(ttmp),               \
		      [ret] "=&r"(ret)                                        \
		    : [addr] "m"(*addr), [mprv] "r"(MSTATUS_MPRV),            \
		      [taddr] "r"((ulong)trap)                                \
		    : "memory");                                              \
		return ret;                                                   \
	}

#define DEFINE_UNPRIVILEGED_STORE_FUNCTION(type, insn)                        \
	void sbi_store_##type(type *addr, type val,                           \
			      struct sbi_scratch *scratch,                    \
			      struct sbi_trap_info *trap)                     \
	{                                                                     \
		register ulong tinfo asm("a3");                               \
		register ulong ttmp asm("a4");                                \
		register ulong mstatus asm("a5");                             \
		register ulong mtvec asm("a6") = sbi_hart_unpriv_trap_addr(); \
		trap->cause = 0;                                              \
		asm volatile(                                                 \
			"add %[tinfo], %[taddr], zero\n"                      \
			"add %[ttmp], %[taddr], zero\n"                       \
			"csrrw %[mtvec], " STR(CSR_MTVEC) ", %[mtvec]\n"      \
			"csrrs %[mstatus], " STR(CSR_MSTATUS) ", %[mprv]\n"   \
			".option push\n"                                      \
			".option norvc\n"                                     \
			#insn " %[val], %[addr]\n"                            \
			".option pop\n"                                       \
			"csrw " STR(CSR_MSTATUS) ", %[mstatus]\n"             \
			"csrw " STR(CSR_MTVEC) ", %[mtvec]"                   \
		    : [mstatus] "+&r"(mstatus), [mtvec] "+&r"(mtvec),         \
		      [tinfo] "+&r"(tinfo), [ttmp] "+&r"(ttmp)                \
		    : [addr] "m"(*addr), [mprv] "r"(MSTATUS_MPRV),            \
		      [taddr] "r"((ulong)trap), [val] "r"(val)                \
		    : "memory");              \
	}

DEFINE_UNPRIVILEGED_LOAD_FUNCTION(u8, lbu)
DEFINE_UNPRIVILEGED_LOAD_FUNCTION(u16, lhu)
DEFINE_UNPRIVILEGED_LOAD_FUNCTION(s8, lb)
DEFINE_UNPRIVILEGED_LOAD_FUNCTION(s16, lh)
DEFINE_UNPRIVILEGED_LOAD_FUNCTION(s32, lw)
DEFINE_UNPRIVILEGED_STORE_FUNCTION(u8, sb)
DEFINE_UNPRIVILEGED_STORE_FUNCTION(u16, sh)
DEFINE_UNPRIVILEGED_STORE_FUNCTION(u32, sw)
#if __riscv_xlen == 64
DEFINE_UNPRIVILEGED_LOAD_FUNCTION(u32, lwu)
DEFINE_UNPRIVILEGED_LOAD_FUNCTION(u64, ld)
DEFINE_UNPRIVILEGED_STORE_FUNCTION(u64, sd)
DEFINE_UNPRIVILEGED_LOAD_FUNCTION(ulong, ld)
#else
DEFINE_UNPRIVILEGED_LOAD_FUNCTION(u32, lw)
DEFINE_UNPRIVILEGED_LOAD_FUNCTION(ulong, lw)

u64 sbi_load_u64(const u64 *addr,
		 struct sbi_scratch *scratch,
		 struct sbi_trap_info *trap)
{
	u64 ret = sbi_load_u32((u32 *)addr, scratch, trap);

	if (trap->cause)
		return 0;
	ret |= ((u64)sbi_load_u32((u32 *)addr + 1, scratch, trap) << 32);
	if (trap->cause)
		return 0;

	return ret;
}

void sbi_store_u64(u64 *addr, u64 val,
		   struct sbi_scratch *scratch,
		   struct sbi_trap_info *trap)
{
	sbi_store_u32((u32 *)addr, val, scratch, trap);
	if (trap->cause)
		return;

	sbi_store_u32((u32 *)addr + 1, val >> 32, scratch, trap);
	if (trap->cause)
		return;
}
#endif

ulong sbi_get_insn(ulong mepc, struct sbi_scratch *scratch,
		   struct sbi_trap_info *trap)
{
	register ulong tinfo asm("a3");
	register ulong ttmp asm("a4");
	register ulong mstatus asm("a5");
	register ulong mtvec asm("a6") = sbi_hart_unpriv_trap_addr();
	ulong insn = 0;

	trap->cause = 0;

	asm volatile(
	    "add %[tinfo], %[taddr], zero\n"
	    "csrrw %[mtvec], " STR(CSR_MTVEC) ", %[mtvec]\n"
	    "csrrs %[mstatus], " STR(CSR_MSTATUS) ", %[mprv]\n"
	    "lhu %[insn], (%[addr])\n"
	    "andi %[ttmp], %[insn], 3\n"
	    "addi %[ttmp], %[ttmp], -3\n"
	    "bne %[ttmp], zero, 2f\n"
	    "lhu %[ttmp], 2(%[addr])\n"
	    "sll %[ttmp], %[ttmp], 16\n"
	    "add %[insn], %[insn], %[ttmp]\n"
	    "2: csrw " STR(CSR_MSTATUS) ", %[mstatus]\n"
	    "csrw " STR(CSR_MTVEC) ", %[mtvec]"
	    : [mstatus] "+&r"(mstatus), [mtvec] "+&r"(mtvec),
	      [tinfo] "+&r"(tinfo), [ttmp] "+&r"(ttmp),
	      [insn] "=&r"(insn)
	    : [mprv] "r"(MSTATUS_MPRV | MSTATUS_MXR),
	      [taddr] "r"((ulong)trap), [addr] "r"(mepc)
	    : "memory");

	switch (trap->cause) {
	case CAUSE_LOAD_ACCESS:
		trap->cause = CAUSE_FETCH_ACCESS;
		trap->tval = mepc;
		break;
	case CAUSE_LOAD_PAGE_FAULT:
		trap->cause = CAUSE_FETCH_PAGE_FAULT;
		trap->tval = mepc;
		break;
	case CAUSE_LOAD_GUEST_PAGE_FAULT:
		trap->cause = CAUSE_FETCH_GUEST_PAGE_FAULT;
		trap->tval = mepc;
		break;
	default:
		break;
	};

	return insn;
}
