/* $Header: screen.h,v 1.1 87/09/11 07:50:07 toddb Exp $ */

/***************************************************************** 
 * DEVICE IDENTIFIERS 
 *****************************************************************/

#define XDEV_XNEST		0	/* X in an X window */

/* DEC address space 199 */
#define XDEV_VS100		1	/* DEC VS100			*/
#define XDEV_QVSS		2	/* DEC QVSS (VS1 and VS2)	*/
#define XDEV_QDSS		3	/* DEC QDSS display		*/
#define XDEV_DECXXX		4	/* reserved for future use	*/
#define XDEV_DECYYY		5	/* reserved for future use	*/
#define XDEV_DECZZZ		6	/* reserved for future use	*/

/* Cognition address space 100 - 199 */
#define XDEV_LEX90		100	/* Lexidata 90, Cognition       */

/* IBM address space 200 - 299 */
#define XDEV_IBMACIS		200	/* IBM ACIS  display, wuf, RT/PC*/
#define XDEV_IBMAPA8		201	/* IBM APA8  display, RT/PC     */
#define XDEV_IBMAPA16		202	/* IBM APA16 display, RT/PC	*/

/* SMI address space 300 - 399 */
#define SUNBASE		300	/* base of SMI displays		*/
#ifndef FBTYPESUN1BW
/* from /usr/include/sun/fbio.h */
#define XFBTYPE_SUN1BW		0
#define XFBTYPE_SUN1COLOR	1
#define XFBTYPE_SUN2BW		2
#define XFBTYPE_SUN2COLOR	3
#define XFBTYPE_SUN2GP		4	/* reserved for future Sun use	*/
#define XFBTYPE_SUN3BW		5	/* reserved for future Sun use	*/
#define XFBTYPE_SUN3COLOR	6	/* reserved for future Sun use	*/
#define XFBTYPE_SUN4BW		7	/* reserved for future Sun use	*/
#define XFBTYPE_SUN4COLOR	8	/* reserved for future Sun use	*/
#define XFBTYPE_NOTSUN1		9	/* reserved for Sun customer	*/
#define XFBTYPE_NOTSUN2		10	/* reserved for Sun customer	*/
#define XFBTYPE_NOTSUN3		11	/* reserved for Sun customer	*/
#endif
#define	XDEV_SUN1BW		FBTYPESUN1BW+SUNBASE
#define	XDEV_SUN1COLOR		FBTYPESUN1COLOR+SUNBASE
#define	XDEV_SUN2BW		FBTYPESUN2BW+SUNBASE
#define	XDEV_SUN2COLOR		FBTYPESUN2COLOR+SUNBASE
#define	XDEV_SUN2GP		FBTYPESUN2GP+SUNBASE
#define	XDEV_SUN3BW		FBTYPESUN3BW+SUNBASE
#define	XDEV_SUN3COLOR		FBTYPESUN3COLOR+SUNBASE
#define	XDEV_SUN4BW		FBTYPESUN4BW+SUNBASE
#define	XDEV_SUN4COLOR		FBTYPESUN4COLOR+SUNBASE
#define	XDEV_NOTSUN1		FBTYPENOTSUN1+SUNBASE
#define	XDEV_NOTSUN2		FBTYPENOTSUN2+SUNBASE
#define	XDEV_NOTSUN3		FBTYPENOTSUN3+SUNBASE

/* MASSCOMP address space 400 - 499 */
#define XDEV_MC1		401	/* Masscomp, in progress	*/
#define XDEV_MC2		402	/* Masscomp, (not implemented)  */
#define XDEV_MC3		403	/* Masscomp, (not implemented)  */

/* Jupiter Systems address space 500 - 599 */
#define XDEV_PGP20		501	/* 24 bit deep frame buffer	*/

