#ifndef	NSIG
#define NSIG	32

#define	SIGHUP	1	/* hangup */
#define	SIGINT	2	/* interrupt */
#define	SIGQUIT	3	/* quit */
#define	SIGILL	4	/* illegal instruction (not reset when caught) */
#ifdef mc68000
#define     ILL_ILLINSTR_FAULT	0x10	/* illegal instruction fault */
#define     ILL_PRIVVIO_FAULT	0x20	/* privilege violation fault */
#define     ILL_COPROCERR_FAULT	0x34	/* [coprocesser protocol error fault] */
#define     ILL_TRAP1_FAULT	0x84	/* trap #1 fault */
#define     ILL_TRAP2_FAULT	0x88	/* trap #2 fault */
#define     ILL_TRAP3_FAULT	0x8c	/* trap #3 fault */
#define     ILL_TRAP4_FAULT	0x90	/* trap #4 fault */
#define     ILL_TRAP5_FAULT	0x94	/* trap #5 fault */
#define     ILL_TRAP6_FAULT	0x98	/* trap #6 fault */
#define     ILL_TRAP7_FAULT	0x9c	/* trap #7 fault */
#define     ILL_TRAP8_FAULT	0xa0	/* trap #8 fault */
#define     ILL_TRAP9_FAULT	0xa4	/* trap #9 fault */
#define     ILL_TRAP10_FAULT	0xa8	/* trap #10 fault */
#define     ILL_TRAP11_FAULT	0xac	/* trap #11 fault */
#define     ILL_TRAP12_FAULT	0xb0	/* trap #12 fault */
#define     ILL_TRAP13_FAULT	0xb4	/* trap #13 fault */
#define     ILL_TRAP14_FAULT	0xb8	/* trap #14 fault */
#endif mc68000
#define	SIGTRAP	5	/* trace trap (not reset when caught) */
#define     TRAP_BKPT	0x1	/* a breakpoint trap */
#define     TRAP_TRACE	0x2	/* a trace trap */
#define	SIGIOT	6	/* IOT instruction */
#define	SIGEMT	7	/* EMT instruction */
#ifdef mc68000
#define     EMT_EMU1010		0x28	/* line 1010 emulator trap */
#define     EMT_EMU1111		0x2c	/* line 1111 emulator trap */
#endif mc68000
#define	SIGFPE	8	/* floating point exception */
#ifdef vax
#define		K_INTOVF 1	/* integer overflow */
#define		K_INTDIV 2	/* integer divide by zero */
#define		K_FLTOVF 3	/* floating overflow */
#define		K_FLTDIV 4	/* floating/decimal divide by zero */
#define		K_FLTUND 5	/* floating underflow */
#define		K_DECOVF 6	/* decimal overflow */
#define		K_SUBRNG 7	/* subscript out of range */
#endif
#ifdef mc68000
#define     FPE_INTDIV_TRAP	0x14	/* integer divide by zero */
#define     FPE_CHKINST_TRAP	0x18	/* CHK [CHK2] instruction */
#define     FPE_TRAPV_TRAP	0x1c	/* TRAPV [cpTRAPcc TRAPcc] instr */
#define     FPE_FLTBSUN_TRAP	0xc0	/* [branch or set on unordered cond] */
#define     FPE_FLTINEX_TRAP	0xc4	/* [floating inexact result] */
#define     FPE_FLTDIV_TRAP	0xc8	/* [floating divide by zero] */
#define     FPE_FLTUND_TRAP	0xcc	/* [floating underflow] */
#define     FPE_FLTOPERR_TRAP	0xd0	/* [floating operand error] */
#define     FPE_FLTOVF_TRAP	0xd4	/* [floating overflow] */
#define     FPE_FLTNAN_TRAP	0xd8	/* [floating Not-A-Number] */
#endif mc68000
#define	SIGKILL	9	/* kill (cannot be caught or ignored) */
#define	SIGKIL	9
#define	SIGBUS	10	/* bus error */
#define	SIGSEGV	11	/* segmentation violation */
#define	SIGSYS	12	/* bad argument to system call */
#define	SIGPIPE	13	/* write on a pipe with no one to read it */
#define	SIGALRM	14	/* alarm clock */
#define	SIGTERM	15	/* software termination signal from kill */

#define	SIGSTOP	17	/* sendable stop signal not from tty */
#define	SIGTSTP	18	/* stop signal from tty */
#define	SIGCONT	19	/* continue a stopped process */
#define	SIGCHLD	20	/* to parent on child stop or exit */
#define	SIGTTIN	21	/* to readers pgrp upon background tty read */
#define	SIGTTOU	22	/* like TTIN for output if (tp->t_local&LTOSTOP) */
#define SIGTINT	23	/* to pgrp on every input character if LINTRUP */
#define	SIGXCPU	24	/* exceeded CPU time limit */
#define	SIGXFSZ	25	/* exceeded file size limit */

#ifndef KERNEL
typedef int	(*SIG_TYP)();
SIG_TYP signal();
#endif

#define	BADSIG		(int (*)())-1
#define	SIG_DFL		(int (*)())0
#define	SIG_IGN		(int (*)())1
#ifdef KERNEL
#define	SIG_CATCH	(int (*)())2
#endif
#define	SIG_HOLD	(int (*)())3

#define	SIGISDEFER(x)	(((int)(x) & 1) != 0)
#define	SIGUNDEFER(x)	(int (*)())((int)(x) &~ 1)
#define	DEFERSIG(x)	(int (*)())((int)(x) | 1)

#define	SIGNUMMASK	0377		/* to extract pure signal number */
#define	SIGDOPAUSE	0400		/* do pause after setting action */
#define	SIGDORTI	01000		/* do ret+rti after setting action */
#endif
