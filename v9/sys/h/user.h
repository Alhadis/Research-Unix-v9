#ifdef KERNEL
#include "../machine/pcb.h"
#include "../machine/reg.h"
#include "../h/dmap.h"
#include "../h/vtimes.h"
#else
#include <machine/reg.h>
#include <machine/pcb.h>
#include <sys/dmap.h>
#include <sys/vtimes.h>
#endif
/*
 * The user structure.
 * One allocated per process.
 * Contains all per process data
 * that doesn't need to be referenced
 * while the process is swapped.
 * The user block is UPAGES*NBPG bytes
 * long; resides at virtual user
 * loc 0x80000000-UPAGES*NBPG; contains the system
 * stack per user; is cross referenced
 * with the proc structure for the
 * same process.
 */
 
#define	SHSIZE	32
#define	NOGROUP	(short)-1		/* end of group marker */

struct	user
{
	char	u_stack[KERNSTACK];	/* the kernel stack */
	struct	pcb u_pcb;
	int	u_arg[5];		/* arguments to current system call */
	label_t	u_qsav;			/* for non-local gotos on interrupts */
	char	u_segflg;		/* 0:user D; 1:system; 2:user I */
	char	u_error;		/* return error code */
	short	u_uid;			/* effective user id */
	short	u_gid;			/* effective group id */
	short	u_ruid;			/* real user id */
	short	u_rgid;			/* real group id */
	short	u_groups[NGROUPS];	/* access group id array */
	struct	proc *u_procp;		/* pointer to proc structure */
	int	*u_ap;			/* pointer to arglist */
	union {				/* syscall return values */
		struct	{
			int	R_val1;
			int	R_val2;
		} u_rv;
#define	r_val1	u_rv.R_val1
#define	r_val2	u_rv.R_val2
		off_t	r_off;
		time_t	r_time;
	} u_r;
	caddr_t	u_base;			/* base address for IO */
	unsigned int u_count;		/* bytes remaining for IO */
	off_t	u_offset;		/* offset in file for IO */
	struct	inode *u_cdir;		/* pointer to inode of current directory */
	struct	inode *u_rdir;		/* root directory of current process */
	char	u_dbuf[DIRSIZ];		/* current pathname component */
	caddr_t	u_dirp;			/* pathname pointer */
	struct	direct u_dent;		/* current directory entry */
/*	struct	inode *u_pdir;		/* inode of parent directory of dirp */
	struct	file *u_ofile[NOFILE];	/* pointers to file structures of open files */
	char	u_pofile[NOFILE];	/* per-process flags of open files */
#define	EXCLOSE 01		/* auto-close on exec */
#define	MMAPPED 02		/* fd used to mmap memory */
	label_t u_ssav;			/* label variable for swapping */
	int	(*u_signal[NSIG])();	/* disposition of signals */
	int	u_code;			/* ``code'' to trap */
/* on SIGILL code passes compatibility mode fault address  */
/* on SIGFPE code passes more specific kind of floating point fault */
	int	*u_ar0;			/* address of users saved R0 */
	struct uprof {			/* profile arguments */
		short	*pr_base;	/* buffer base */
		unsigned pr_size;	/* buffer size */
		unsigned pr_off;	/* pc offset */
		unsigned pr_scale;	/* pc scaling */
	} u_prof;
	char	u_eosys;		/* special action on end of syscall */
	char	u_sep;			/* flag for I and D separation */
	dev_t	u_ttydev;		/* dev,ino of controlling tty */
	ino_t	u_ttyino;
	union {
	   struct {			/* header of executable file */
#ifdef sun
		unsigned short Ux_mach;	/* machine type */
		unsigned short Ux_mag;	/* magic number */
#else
		unsigned Ux_mag;	/* magic number */
#endif
		unsigned Ux_tsize;	/* text size */
		unsigned Ux_dsize;	/* data size */
		unsigned Ux_bsize;	/* bss size */
		unsigned Ux_ssize;	/* symbol table size */
		unsigned Ux_entloc;	/* entry location */
		unsigned Ux_unused;
		unsigned Ux_relflg;
	   } Ux_A;
	   char ux_shell[SHSIZE];	/* #! and name of interpreter */
	} u_exdata;
#ifdef sun
#define	ux_mach		Ux_A.Ux_mach
#define M_OLDSUN2	0	/* old sun-2 executable files */
#define M_68010		1	/* runs on either 68010 or 68020 */
#define M_68020		2	/* runs only on 68020 */

#define	OMAGIC	0407		/* old impure format */
#define	NMAGIC	0410		/* read-only text */
#define	ZMAGIC	0413		/* demand load format */
#endif
#define	ux_mag		Ux_A.Ux_mag
#define	ux_tsize	Ux_A.Ux_tsize
#define	ux_dsize	Ux_A.Ux_dsize
#define	ux_bsize	Ux_A.Ux_bsize
#define	ux_ssize	Ux_A.Ux_ssize
#define	ux_entloc	Ux_A.Ux_entloc
#define	ux_unused	Ux_A.Ux_unused
#define	ux_relflg	Ux_A.Ux_relflg

	char	u_comm[DIRSIZ];
	time_t	u_start;
	char	u_acflag;
	short	u_fpflag;		/* unused now, will be later */
	short	u_cmask;		/* mask for file creation */
	size_t	u_tsize;		/* text size (clicks) */
	size_t	u_dsize;		/* data size (clicks) */
	size_t	u_ssize;		/* stack size (clicks) */
	struct	vtimes u_vm;		/* stats for this proc */
	struct	vtimes u_cvm;		/* sum of stats for reaped children */
	struct	dmap u_dmap;		/* disk map for data segment */
	struct	dmap u_smap;		/* disk map for stack segment */
	struct	dmap u_cdmap, u_csmap;	/* shadows of u_dmap, u_smap, for
					   use of parent during fork */
	time_t	u_outime;		/* user time at last sample */
	size_t	u_odsize, u_ossize;	/* for (clumsy) expansion swaps */
	int	u_limit[8];		/* see <sys/vlimit.h> */
	int	u_nbadio;		/* # IO operations on hungup streams */
	char	u_logname[8];		/* login name */
#ifdef sun
	int	u_lofault;		/* catch faults in locore.s */
	struct	hole {			/* a data space hole (no swap space) */
		int	uh_first;	/* first data page in hole */
		int	uh_last;	/* last data page in hole */
	} u_hole;
	/* sun2 only */
	int	u_memropc[12];		/* state of ropc */
	struct skyctx {
		u_int	 usc_regs[8];	/* the Sky registers */
		short	 usc_cmd;	/* current command */
		short	 usc_used;	/* user is using Sky */
	} u_skyctx;
	/* end sun2 only */
	/* 68020/68881 only */
	struct fp_status u_fp_status;	/* user visible fpp state */
	struct fp_istate u_fp_istate;	/* internal fpp state */
	/* end 68020/68881 only */
	/* 68020/fpa only */
        struct fpa_istate u_fpa_istate; /* supervisor privilege regs  */
	struct fpa_status u_fpa_status; /* user privilege regs    */
	/* end 68020/fpa only */
#endif sun
};

/* u_eosys values */
#define	JUSTRETURN	0
#define	RESTARTSYS	1
#define	SIMULATERTI	2
#define	REALLYRETURN	3

/* u_error codes */
#ifdef KERNEL
#include "../h/errno.h"
#else
#include <errno.h>
#endif

#ifdef KERNEL
#ifdef sun
#define	u	(*(struct user *)UADDR)
#else sun
extern	struct user u;
#endif sun
extern	struct user swaputl;
extern	struct user forkutl;
extern	struct user xswaputl;
extern	struct user xswap2utl;
extern	struct user pushutl;
extern	struct user vfutl;
extern	struct user prusrutl;
#endif
