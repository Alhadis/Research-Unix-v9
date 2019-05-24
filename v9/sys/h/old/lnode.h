/*
**	Structure for active limits.
**
**	Used by the kernel and by programs such as "init" and "login".
*/

typedef short	uid_t;

struct lnode
{
	uid_t		l_uid;		/* real uid for owner of this node */
	u_short		l_flags;	/* (see below) */
	u_short		l_shares;	/* allocated shares */
	uid_t		l_group;	/* uid for this node's scheduling group */
	float		l_usage;	/* decaying accumulated costs */
	float		l_charge;	/* long term accumulated costs */
};

/*
**	Values of limits "flags".
*/

#define	ACTIVELNODE	001	/* this lnode is on active list */
#define	LASTREF		002	/* set for L_DEADLIM if last reference to this lnode */
#define	DEADGROUP	004	/* group account is dead */
#define	GROUPUSAGE	010	/* this lnode's usage is affected by group */
#define	CHNGDLIMITS	020	/* this lnode's limits have changed */
#define	NOTSHARED	040	/* this lnode does not get a share of the m/c */

/*
**	Kernel user limit structure.
*/

typedef struct kern_lnode *	KL_p;

struct kern_lnode
{
	KL_p		kl_next;	/* next in active list */
	KL_p		kl_prev;	/* prev in active list */
	KL_p		kl_parent;	/* group parent */
	KL_p		kl_gnext;	/* next in parent's group */
	KL_p		kl_ghead;	/* start of this group */
	struct lnode	kl;		/* the limits */
	float		kl_norms;	/* normalised shares for this lnode */
	float		kl_eshare;	/* effective share for this group */
	float		kl_gshares;	/* total shares for this group */
	float		kl_temp;	/* temporary for scheduler */
	u_long		kl_cost;	/* cost accumulating in current period */
	float		kl_rate;	/* run rate for this lnode */
	u_short		kl_refcount;	/* processes attached to this lnode */
	u_short		kl_children;	/* lnodes attached to this lnode */
	u_long		kl_muse;	/* actual number of pages used */
};

/*
**	Limits system call defines.
*/

#define	L_MYLIM		0	/* get own limits structure */
#define	L_OTHLIM	1	/* get other user's limits structure */
#define	L_ALLLIM	2	/* get all active limits structures */
#define	L_SETLIM	3	/* set limits */
#define	L_DEADLIM	4	/* get zombie and limits structure of child */
#define	L_CHNGLIM	5	/* change active lnode (limits only) */
#define	L_DEADGROUP	6	/* get limits structure for dead group */
#define	L_GETCOSTS	7	/* get share constants */
#define	L_SETCOSTS	8	/* set share constants */
#define	L_MYKN		9	/* get own kernel lnode structure */
#define	L_OTHKN		10	/* get other user's kernel lnode structure */
#define	L_ALLKN		11	/* get all active kernel lnode structures */

/*
**	Kernel lnode table.
*/

#ifdef	KERNEL
extern KL_p	lnodes;
extern KL_p	lnodesMAXUSERS;
extern KL_p	lastlnode;
extern int	maxusers;

extern float	zerof;
extern float	onef;
extern float	twof;
#endif	KERNEL

#define	_LNODE_	1
