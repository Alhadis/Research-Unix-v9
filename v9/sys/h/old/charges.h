/*
**	Share scheduling charges
*/

struct sh_consts	shconsts =
{
	 NOSHARE	/* Flags */
	,4		/* Run rate for scheduler */
	,60		/* Max. users */
	,4		/* Max. group nesting */
	,0.9659363	/* Decay rate for ``kl_rate'' (20 sec avg. 1/2 life) */
	,1e38		/* Max. absolute priority (ref. clock.c)*/
	,1e28		/* Max. priority for a normal process */
	,1e12		/* Max. usage */
	,0.97715998	/* Decay rate for ``l_usage'' (120 sec 1/2 life) */
	,256		/* Decay base for ``p_sharepri'' (10 sec avg. 1/2 life) */
	,0		/* Syscall */
	,0		/* Bio */
	,0		/* Tio */
	,1000		/* Tick */
	,0		/* Click */
	,0.99826944	/* Decay rate for max nice ``p_sharepri'' (400 sec 1/2 life) */
	,2.0		/* Factor for max effective user share */
	,0.9		/* Factor for min effective group share */
};
