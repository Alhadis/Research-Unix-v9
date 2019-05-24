/*
 * hypot -- sqrt(p*p+q*q), but overflows only if the result does.
 * See Cleve Moler and Donald Morrison,
 * ``Replacing Square Roots by Pythagorean Sums,''
 * IBM Journal of Research and Development,
 * Vol. 27, Number 6, pp. 577-581, Nov. 1983
 */
double hypot(p, q)
double p, q;
{
	double r, s;
	if(p<0) p = -p;
	if(q<0) q = -q;
	if(p<q){ r=p; p=q; q=r; }
	if(p==0) return 0;
	for(;;){
		r=q/p;
		r*=r;
		s=r+4;
		if(s==4)
			return p;
		r/=s;
		p+=2*r*p;
		q*=r;
	}
}
