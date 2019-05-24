/*
**	Share scheduling parameters
*/

struct sh_consts
{
	/** Parameters **/

	u_short	sc_fl;		/* Scheduling flags */
	u_short	sc_delta;	/* Run rate for scheduler in secs. */
	u_short	sc_maxusers;	/* Max. number of active users */
	u_short	sc_maxgroups;	/* Max. group nesting */
	u_long	sc_sparep0;	/* <spare> */
	float	sc_maxpri;	/* Max. absolute priority */
	float	sc_maxupri;	/* Max. priority for a normal process */
	float	sc_maxusage;	/* Max. usage considered */
	float	sc_decay;	/* Decay factor for ``l_usage'' */
	u_long	sc_pridecbase;	/* Base for decay for ``p_sharepri'' */
	u_long	sc_syscall;	/* Cost of system call */
	u_long	sc_bio;		/*   "  "  logical block i/o */
	u_long	sc_tio;		/*   "  "  stream i/o */
	u_long	sc_tick;	/*   "  "  cpu tick */
	u_long	sc_click;	/*   "  "  memory tick */
	float	sc_pridecay;	/* Decay rate for maximally niced processes */
	float	sc_maxushare;	/* Factor for max effective user share */
	float	sc_mingshare;	/* Factor for min effective group share */
	u_long	sc_sparep[1];	/* <spare> */

	/** Feedback **/

	float	sc_totusage;	/* E{1/kl.l_usage} */
	float	sc_sparef0;	/* <spare> */
	u_short	sc_users;	/* Number of active users */
	u_short	sc_groups;	/* Number of active groups */
	float	sc_highshpri;	/* High value of p_sharepri */
	float	sc_maxcusage;	/* Max. current usage */
	u_long	sc_syscallc;	/* Total counts of system calls */
	u_long	sc_bioc;	/*   "     "    "  logical block i/o */
	u_long	sc_tioc;	/*   "     "    "  stream i/o */
	u_long	sc_tickc;	/*   "     "    "  cpu ticks */
	u_long	sc_clickc;	/*   "     "    "  memory ticks */
	u_long	sc_sparef[4];	/* <spare> */
};

extern struct sh_consts	shconsts;

#define	DecayUsage	shconsts.sc_decay
#define	Delta		shconsts.sc_delta
#define	Groups		shconsts.sc_groups
#define	LASTPARAM	shconsts.sc_sparep[0]
#define	MAXGROUPS	shconsts.sc_maxgroups
#define	MAXPRI		shconsts.sc_maxpri
#define	MaxSharePri	shconsts.sc_highshpri
#define	MAXUPRI		shconsts.sc_maxupri
#define	MaxUsage	shconsts.sc_maxcusage
#define	MAXUSAGE	shconsts.sc_maxusage
#define	MAXUSERS	shconsts.sc_maxusers
#define	MAXUSHARE	shconsts.sc_maxushare
#define	MINGSHARE	shconsts.sc_mingshare
#define	PriDecay	shconsts.sc_pridecay
#define	PriDecayBase	shconsts.sc_pridecbase
#define	Shareflags	shconsts.sc_fl
#define	TotUsage	shconsts.sc_totusage
#define	Users		shconsts.sc_users

/*
**	Share scheduling flags
*/

#define	NOSHARE		01	/* Don't run scheduler at all */
#define	ADJGROUPS	02	/* Adjust group usages */
#define	LIMSHARE	04	/* Limit maximum share */

#ifdef	KERNEL
/*
**	Request software interrupt to run scheduler
*/

#ifdef vax
#define	setshsched()	mtpr(SIRR, 0x1)
#define	splshsched()	splx(0x1)
#endif
#ifdef sun
#define	setshsched()	softcall(share, (caddr_t)0);
#define	splshsched()	spl1()
#endif

/*
**	Array of costs for a cpu tick biased by p_nice values.
*/

extern long		NiceTicks[];

/*
**	Table for pre-calculated priority decays
*/

extern float		NiceDecays[];
#endif	KERNEL

#define	_SHARE_		1
