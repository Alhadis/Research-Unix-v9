#include <sys/vm.h>

#ifndef	PAGSIZ
#define	PAGSIZ	(NBPG*CLSIZE)
#endif

#define	MAXSTOR KERNELBASE
#define	MAXFILE 0xffffffff

/*
  * INKERNEL tells whether its argument is a kernel space address.
 */
#define	INKERNEL(x)	(((x)&0x0F000000) == 0x0F000000)

#define	KERNOFF		0x0F000000
