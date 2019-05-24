/*	@(#)fbio.h 1.1 86/02/03 SMI	*/

/*
 * Frame buffer descriptor.
 * Returned by FBIOGTYPE ioctl on frame buffer devices.
 */
struct	fbtype {
	int	fb_type;	/* as defined below */
	int	fb_height;	/* in pixels */
	int	fb_width;	/* in pixels */
	int	fb_depth;	/* bits per pixel */
	int	fb_cmsize;	/* size of color map (entries) */
	int	fb_size;	/* total size in bytes */
};

#define	FBTYPE_SUN1BW		0
#define	FBTYPE_SUN1COLOR	1
#define	FBTYPE_SUN2BW		2
#define	FBTYPE_SUN2COLOR	3
#define	FBTYPE_SUN2GP		4	/* reserved for future Sun use */
#define	FBTYPE_SUN3BW		5	/* reserved for future Sun use */
#define	FBTYPE_SUN3COLOR	6	/* reserved for future Sun use */
#define	FBTYPE_SUN4BW		7	/* reserved for future Sun use */
#define	FBTYPE_SUN4COLOR	8	/* reserved for future Sun use */
#define	FBTYPE_NOTSUN1		9	/* reserved for customer */
#define	FBTYPE_NOTSUN2		10	/* reserved for customer */
#define	FBTYPE_NOTSUN3		11	/* reserved for customer */

#define	FBIOGTYPE (('F'<<8)|1)
#define	KBIOISKBD (('K'<<8)|1)
