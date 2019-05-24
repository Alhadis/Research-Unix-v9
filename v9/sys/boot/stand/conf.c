#include <machine/sunromvec.h>

extern struct boottab xydriver;
extern struct boottab sddriver;
extern struct boottab stdriver;
extern struct boottab mtdriver;
extern struct boottab xtdriver;

/*
 * The device table 
 */
struct boottab *(devsw[]) = {
	&sddriver,
	&stdriver,
	(struct boottab *)0,
};
