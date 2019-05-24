static
short costab[91]={
	1024,	1024,	1023,	1023,	1022,
	1020,	1018,	1016,	1014,	1011,
	1008,	1005,	1002,	998,	994,
	989,	984,	979,	974,	968,
	962,	956,	949,	943,	935,
	928,	920,	912,	904,	896,
	887,	878,	868,	859,	849,
	839,	828,	818,	807,	796,
	784,	773,	761,	749,	737,
	724,	711,	698,	685,	672,
	658,	644,	630,	616,	602,
	587,	573,	558,	543,	527,
	512,	496,	481,	465,	449,
	433,	416,	400,	384,	367,
	350,	333,	316,	299,	282,
	265,	248,	230,	213,	195,
	178,	160,	143,	125,	107,
	89,	71,	54,	36,	18,
	0,
};
cos (x)
register x;
{
	x %= 360;
	if(x<0)
		x+=360;
	if(x<=180)
		return(x<90? costab[x] : -costab[180-x]);
	return(x<180+90? -costab[x-180] : costab[360-x]);
}

sin (x)
register x;
{
	return(cos(x-90));
}

static
qatan2 (x, y)
register x, y;
{
	if(x<y)
		return(90-(45*((long)x)/y));
	if(y==0)
		return(0);
	return(45*((long)y)/x);
}

atan2 (xx, yy){
	register x, y;
	x = abs(xx);
	y = abs(yy);
	if(xx>=0 && yy>=0)
		return(qatan2(x, y));
	if(xx<0 && yy<=0)
		return(180+qatan2(x, y));
	if(xx<0 && yy>0)
		return(180-qatan2(x, y));
	return(360-qatan2(x, y));
}

norm (x,y,z)
{
	return (sqrt(x*x + y*y + z*z));
}

sqrtryz (x,y,z)
{
	register long sumsq;

	sumsq = x*x - y*y - z*z;
	if(sumsq <= 0)
		return 0;
	return(sqrt(sumsq));
}

#define MAXROOT 0xb504
sqrt (x)
register long x;
{
	register long high = MAXROOT;
	register long low = 0;
	register long current = MAXROOT/2;
	if(x <= 0)
		return 0;
	if(x >= MAXROOT*MAXROOT)
		return(MAXROOT);
	while(high>low+1){
		if(current*current==x)
			return (current);
		if(current*current>x)
			high=current;
		else
			low=current;
		current=(high+low)>>1;
	}
	return(current);
}
