/*
**	Share scheduler
*/

#include	"../h/param.h"
#include	"../h/lnode.h"
#include	"../h/share.h"
#include	"../h/charges.h"


float	zerof	= 0.0;
float	onef	= 1.0;
float	twof	= 2.0;

static void	adjgroups();



/*
**	Assumes that a group parent has been inserted into list before any group member.
**	List is rooted in ``lnodes[0]''.
*/

share()
{
	register KL_p	lp;
	register float	maxusage;
	register float	f;
	register float	g;
	register float	one = onef;
	register float	zero = zerof;

	Users = 0;
	Groups = 1;
	TotUsage = zero;
	MaxUsage = one;

	/*
	**	Special treatment for root.
	*/

	lp = &lnodes[0];
	f = lp->kl_cost;
	lp->kl_cost = 0;
	lp->kl.l_charge += f;
	lp->kl_temp += f;	/* Root's node accumulates all charges here */
	lp->kl.l_usage = one;	/* Root is not shared */

	if ( Shareflags & NOSHARE )
		return;

	/*
	**	Scan up active list and accumulate charges.
	*/

	for ( lp = lastlnode ; lp->kl_prev != (KL_p)0 ; lp = lp->kl_prev )
	{
		lp->kl_cost += lp->kl_muse * shconsts.sc_click;	/* Charge for memory */
		shconsts.sc_clickc += lp->kl_muse;		/* Count memory click */
		lp->kl_temp += lp->kl_cost;
		lp->kl.l_charge += lp->kl_temp;
		lp->kl_parent->kl_temp += lp->kl_temp;
	}

	/*
	**	Adjust costs for groups that are receiving too small a share.
	*/

	if ( Shareflags & ADJGROUPS )
		adjgroups(lnodes, one, 0);

	/*
	**	Scan down active list and process usage.
	*/

	maxusage = one;

	for ( lp = &lnodes[0] ; (lp = lp->kl_next) != (KL_p)0 ; )
	{
		if ( lp->kl.l_flags & NOTSHARED )
			Groups++;
		else
			Users++;

		/*
		**	Accumulate usage. (Background users have no shares.)
		*/

		if ( lp->kl_norms )
		{
			lp->kl.l_usage *= DecayUsage;

			if ( lp->kl_ghead == (KL_p)0 )
				f = lp->kl_temp / lp->kl_norms;
			else
			if ( lp->kl.l_flags & GROUPUSAGE )
				f = lp->kl_temp / (lp->kl_eshare * lp->kl_eshare);
			else
				f = (float)lp->kl_cost / lp->kl_norms;

			if ( (f += lp->kl.l_usage) < twof )
				f = twof;	/* The minimum */

			lp->kl.l_usage = f;

			if ( f > maxusage && f < MAXUSAGE )
				maxusage = f;	/* Remember max. for process priority normalisation */

			TotUsage += one / f;
		}
		else
			lp->kl.l_usage = MAXUSAGE;	/* setpri() treats these specially */

		lp->kl_temp = zero;
		lp->kl_cost = 0;
	}

	MaxUsage = maxusage;	/* Export value */

	/*
	**	Limit usage so that no user has more than MAXUSHARE * <allocated share>.
	*/

	if ( !(Shareflags & LIMSHARE) || TotUsage == zero )
		return;

	maxusage = MAXUSAGE / twof;

	g = one / (TotUsage * MAXUSHARE);

	for ( lp = &lnodes[0] ; (lp = lp->kl_next) != (KL_p)0 ; )
		if
		(
			lp->kl.l_usage < maxusage
			&&
			(f = g / (lp->kl.l_usage * lp->kl_eshare)) > one
			&&
			(f *= lp->kl.l_usage) < maxusage
		)
		{
			TotUsage -= one / lp->kl.l_usage;
			lp->kl.l_usage = f;
			TotUsage += one / f;

			g = one / (TotUsage * MAXUSHARE);
		}
}



/*
**	Decrease costs for groups getting less than their share.
*/

static void
adjgroups(gl, aa, d)
	KL_p		gl;
	float		aa;
	int		d;
{
	register KL_p	lp;
	register float	f;
	register float	totcharges = zerof;
	register u_long	totshares = 0;
	register float	a = aa;

	for ( lp = gl->kl_ghead ; lp != (KL_p)0 ; lp = lp->kl_gnext )
	{
		totcharges += (lp->kl_temp *= a);
		totshares += lp->kl.l_shares;
	}

	if ( d >= MAXGROUPS || totshares == 0 || totcharges == zerof )
		return;

	totcharges *= MINGSHARE;
	totcharges /= totshares;

	for ( lp = gl->kl_ghead ; lp != (KL_p)0 ; lp = lp->kl_gnext )
	{
		if ( lp->kl_ghead == (KL_p)0 || lp->kl.l_shares == 0 )
			continue;

		lp->kl_cost *= a;	/* Scale cost in case GROUPUSAGE */

		if ( (f = lp->kl_temp / (lp->kl.l_shares * totcharges)) < onef )
			f *= a;
		else
			f = a;

		adjgroups(lp, f, d+1);
	}
}



/*
**	Decay run rate.
*/

void
decayrate()
{
	register KL_p	lp;
	register float	one = onef;

	for ( lp = lnodes ; lp != (KL_p)0 ; lp = lp->kl_next )
		if ( (lp->kl_rate *= DecayRate) < one )
			lp->kl_rate = one;
}
