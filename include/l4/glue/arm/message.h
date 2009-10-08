/*
 * Userspace thread control block
 *
 * Copyright (C) 2007-2009 Bahadir Bilgehan Balban
 */
#ifndef __GLUE_ARM_MESSAGE_H__
#define __GLUE_ARM_MESSAGE_H__

/*
 * Here's a summary of how ARM registers are used during IPC:
 *
 * System registers:
 * r0 - r2: Passed as arguments to ipc() call. They are the registers
 * the microkernel will read and they have system-wide meaning.
 *
 * Primary message registers:
 * r3 - r8: These 6 registers are the primary message registers MR0-MR6
 * Their format is application-specific, i.e. the microkernel imposes no
 * format restrictions on them.
 *
 * TODO: The only exception is that, for ANYTHREAD receivers the predefined
 * MR_SENDER is touched by the kernel to indicate the sender. This register
 * is among the primary MRs and it may be better fit to put it into one of
 * the system registers.
 *
 * l4lib registers: (MR_TAG, MR_SENDER, MR_RETURN)
 * Some of the primary message registers are used by the l4lib convenience
 * library for operations necessary on most or all common ipcs. For example
 * every ipc has a tag that specifies the ipc reason. Also send/receive
 * operations require a return value. Threads that are open to receive from
 * all threads require the sender id. These values are passed in predefined
 * primary message registers, but the microkernel has no knowledge about them.
 *
 * System call registers: L4SYS_ARG0 to ARG4.(See syslib.h for definitions)
 * Finally the rest of the primary message registers are available for
 * implementing system call arguments. For example the POSIX services use
 * these arguments to pass posix system call information.
 *
 * Secondary Message Registers:
 * These are non-real registers and are present in the UTCB memory region.
 * Both real and non-real message registers have a location in the UTCB, but
 * non-real ones are copied only if the FULL IPC flag is set.
 *
 * The big picture:
 *
 * r0	System register
 * r1	System register
 * r2	System register
 * r3	Primary MR0	MR_RETURN, MR_TAG	Present in UTCB, Short IPC
 * r4	Primary MR1	MR_SENDER		Present in UTCB, Short IPC
 * r5	Primary MR2	L4SYS_ARG0		Present in UTCB, Short IPC
 * r6	Primary MR3	L4SYS_ARG1		Present in UTCB, Short IPC
 * r7	Primary MR4	L4SYS_ARG2		Present in UTCB, Short IPC
 * r8 	Primary MR5	L4SYS_ARG3		Present in UTCB, Short IPC
 * x	Secondary MR6				Present in UTCB, Full IPC only
 * x	Secondary MR64				Present in UTCB, Full IPC only
 *
 * Complicated for you? Suggest a simpler design and it shall be implemented!
 */

#define MR_REST			((UTCB_SIZE >> 2) - MR_TOTAL - 2)	/* -2 is for fields on utcb */
#define MR_TOTAL		6
#define MR_TAG			0	/* Contains the purpose of message */
#define MR_SENDER		1	/* For anythread receivers to discover sender */
#define MR_RETURN		0	/* Contains the posix return value. */

/* These define the mr start - end range that isn't used by userspace syslib */
#define MR_UNUSED_START		2	/* The first mr that's not used by syslib.h */
#define MR_UNUSED_TOTAL		(MR_TOTAL - MR_UNUSED_START)
#define MR_USABLE_TOTAL		MR_UNUSED_TOTAL

/* These are defined so that we don't hard-code register names */
#define MR0_REGISTER		r3
#define MR_RETURN_REGISTER	r3

#define L4_IPC_FLAGS_SHORT		0x00000000	/* Short IPC involves just primary message registers */
#define L4_IPC_FLAGS_FULL		0x00000001	/* Full IPC involves full UTCB copy */
#define L4_IPC_FLAGS_EXTENDED		0x00000002	/* Extended IPC can page-fault and copy up to 2KB */
#define L4_IPC_FLAGS_MSG_INDEX_MASK	0x00000FF0	/* Index of message register with buffer pointer */
#define L4_IPC_FLAGS_TYPE_MASK		0x0000000F
#define L4_IPC_FLAGS_SIZE_MASK		0x0FFF0000
#define L4_IPC_FLAGS_SIZE_SHIFT		16
#define L4_IPC_FLAGS_MSG_INDEX_SHIFT	4

#define L4_IPC_EXTENDED_MAX_SIZE	(SZ_1K*2)

/* Primaries aren't used for memcopy. Those ops use this as a parameter */
#define L4_UTCB_FULL_BUFFER_SIZE	(MR_REST * sizeof(int))

#include INC_GLUE(memlayout.h)

#if defined (__KERNEL__)

/* Kernel-only flags */
#define IPC_FLAGS_SHORT			L4_IPC_FLAGS_SHORT
#define IPC_FLAGS_FULL			L4_IPC_FLAGS_FULL
#define IPC_FLAGS_EXTENDED		L4_IPC_FLAGS_EXTENDED
#define IPC_FLAGS_MSG_INDEX_MASK	L4_IPC_FLAGS_MSG_INDEX_MASK
#define IPC_FLAGS_TYPE_MASK		L4_IPC_FLAGS_TYPE_MASK
#define IPC_FLAGS_SIZE_MASK		L4_IPC_FLAGS_SIZE_MASK
#define IPC_FLAGS_SIZE_SHIFT		L4_IPC_FLAGS_SIZE_SHIFT
#define IPC_FLAGS_MSG_INDEX_SHIFT	L4_IPC_FLAGS_MSG_INDEX_SHIFT
#define IPC_FLAGS_ERROR_MASK		0xF0000000
#define IPC_FLAGS_ERROR_SHIFT		28
#define IPC_EFAULT			(1 << 28)
#define IPC_ENOIPC			(1 << 29)

#define IPC_EXTENDED_MAX_SIZE		(SZ_1K*2)

struct utcb {
	u32 mr[MR_TOTAL];	/* MRs that are mapped to real registers */
	u32 saved_tag;		/* Saved tag field for stacked ipcs */
	u32 saved_sender;	/* Saved sender field for stacked ipcs */
	u32 mr_rest[MR_REST];	/* Complete the utcb for up to 64 words */
};
#endif

#endif /* __GLUE_ARM_MESSAGE_H__ */
