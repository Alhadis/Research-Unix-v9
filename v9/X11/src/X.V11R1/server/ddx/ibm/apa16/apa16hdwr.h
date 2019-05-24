#ifndef HDWR_SEEN
#define HDWR_SEEN 1
/***********************************************************
		Copyright IBM Corporation 1987

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of IBM not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* $Header: apa16hdwr.h,v 5.2 87/09/13 03:22:12 erik Exp $ */
/* $Source: /u1/X11/server/ddx/ibm/apa16/RCS/apa16hdwr.h,v $ */

#if !defined(lint) && !defined(LOCORE) && defined(RCS_HDRS)
static char *rcsidapa16hdwr = "$Header: apa16hdwr.h,v 5.2 87/09/13 03:22:12 erik Exp $";
#endif

#define	APA16_BASE		(0xf4d80000)
#define SCREEN_ADDR(x,y)	((CARD16 *)(APA16_BASE+((y)*(1024/8))+((x)/8)))

#define	APA16_WIDTH		1024
#define	APA16_HEIGHT		768

extern	int	apa16_qoffset;
extern	CARD16	apa16_rop2stype[];

#define CURSOR_X	(*(CARD16 *)0xf4d9f800)
#define CURSOR_Y	(*(CARD16 *)0xf4d9f802)

#define CURSOR_AREA_TOP		784
#define CURSOR_AREA_BOTTOM	(784+CURSOR_HEIGHT-1)
#define CURSOR_AND_OFFSET	0
#define CURSOR_XOR_OFFSET	48
#define	CURSOR_WIDTH		48
#define	CURSOR_HEIGHT		64

#define ACTIVE_AND_AREA	(SCREEN_ADDR(0,CURSOR_AREA_TOP))
#define ACTIVE_XOR_AREA	(SCREEN_ADDR(CURSOR_XOR_OFFSET,CURSOR_AREA_TOP))

#define	FONT_TOP	848
#define	FONT_BOTTOM	895	

#define CSR		(*(CARD16 *)0xf0000d12)
#define	BLACK_ON_WHITE()	(CSR|=0x400)
#define	WHITE_ON_BLACK()	(CSR&=~0x400)
#define	TOGGLE_BACKGRND()	(CSR^=0x400)

#define	MR		(*(CARD16 *)0xf0000d10)
#define MODE_SHADOW	(*(CARD16 *)0xf4d9f812)

#define	MERGE_MODE_MASK	(0xff0f)
#define	MERGE_MODE	(MODE_SHADOW&MERGE_MODE_MASK)
#define	SET_MERGE_MODE(mode)	(MR=(MODE_SHADOW&MERGE_MODE_MASK)|(mode))
#define	MERGE_BLACK	(0x20)
#define	MERGE_COPY	(0x90)
#define	MERGE_INVERT	(0xa0)
#define	MERGE_WHITE	(0xb0)

#define	BRANCH_SIZE	4
#define QPTR_QUEUE_TOP	(SPTR_TO_QPTR(QUEUE_TOP))
#define QUEUE_TOP	((CARD16 *)(SCREEN_ADDR(APA16_WIDTH-1,1006)))
#define QUEUE_BASE	((CARD16 *)(SCREEN_ADDR(0,849)))
#define QUEUE_SIZE	(((int)QUEUE_TOP)-((int)QUEUE_BASE))
#define	QUEUE_CNTR	(*(CARD16 *)0xf4d9f804)
#define QUEUE_PTR	(*(CARD16 *)0xf4d9f806)
#define INCR_QUEUE_CNTR()	((*(CARD16 *)0xf0000d14)=0)

#define SPTR_TO_QPTR(p)	((((unsigned)(p))>>1)&0xffff)
#define QPTR_TO_SPTR(p)	((CARD16 *)(0xf4d90000|((((unsigned)(p))<<1)&0xffff)))
#define SET_QUEUE_PTR(p)	(QUEUE_PTR=SPTR_TO_QPTR(p))

#define LOAD_QUEUE(rop,off)	((*(QUEUE_BASE+apa16_qoffset+(off)))=(rop))

#define REG_LOAD(reg,val,off)	\
	LOAD_QUEUE((((reg)<<12)&0xf000)|(val)&0x3ff,off)

#define	CMD_MASK	0xfff0
#define STYPE_MASK	0x000f
#define EXEC_MASK	0x0800

#define STYPE_BLACK		0x0000
#define STYPE_WHITE		0x000f
#define STYPE_RECT_INVERT	0x0009
#define STYPE_INVERT		0x000a
#define STYPE_NOP		0x0007

#define ROP_RECT_FILL		0xd2f0
#define ROP_RECT_COPY		0xd300
#define ROP_RECT_COPY_L90	0xd310
#define ROP_RECT_COPY_R90	0xd320
#define ROP_RECT_COPY_X180	0xd330
#define ROP_RECT_COPY_Y180	0xd340
#define ROP_VECTOR		0xd350
#define ROP_NULL_VECTOR		0xd360
#define	ROP_BRANCH		0xd3b0
#define	ROP_VIDEO_ON		0xd3e0
#define	ROP_VIDEO_OFF		0xd3f0


#define CHECK_QUEUE(off,inst) \
	((apa16_qoffset-(off)<BRANCH_SIZE?\
		(REG_LOAD(0xa,((QPTR_QUEUE_TOP>>8)&0xff),0),\
		 REG_LOAD(0xb,(QPTR_QUEUE_TOP&0xff),1),\
		 LOAD_QUEUE(ROP_BRANCH,2),\
		 apa16_qoffset= (QUEUE_SIZE/2)-(off)):\
		 apa16_qoffset-= (off)),(inst))


#define APA16_GO()	(((*(QUEUE_BASE+apa16_qoffset+1))|=EXEC_MASK),\
				INCR_QUEUE_CNTR())

#define QUEUE_INIT()	((apa16_qoffset=QUEUE_SIZE/2),\
			    SET_QUEUE_PTR(QUEUE_TOP))

#define QUEUE_RESET()	{if (!QUEUE_CNTR) {QUEUE_INIT();}\
			 else if (QUEUE_CNTR>1000) {while (QUEUE_CNTR>700);}}

#define APA16_GET_CMD(cmd,rrop,out) \
	if (((cmd)&CMD_MASK)==ROP_RECT_FILL) {\
	    (out)= ((cmd)&CMD_MASK)|\
		(((rrop)==RROP_WHITE?STYPE_WHITE:\
		    ((rrop)==RROP_BLACK?STYPE_BLACK:\
		    ((rrop)==RROP_INVERT?\
			STYPE_RECT_INVERT:\
			STYPE_NOP)))&STYPE_MASK);\
	}\
	else if ((((cmd)&CMD_MASK)==ROP_VECTOR)||\
		 (((cmd)&CMD_MASK)==ROP_NULL_VECTOR)) {\
	    (out)= ((cmd)&CMD_MASK)|\
		(((rrop)==RROP_WHITE?STYPE_WHITE:\
		    ((rrop)==RROP_BLACK?STYPE_BLACK:\
		    ((rrop)==RROP_INVERT?\
			STYPE_INVERT:\
			STYPE_NOP)))&STYPE_MASK);\
	}\
	else if ((((cmd)&CMD_MASK)==ROP_RECT_COPY)||\
		 (((cmd)&CMD_MASK)==ROP_RECT_COPY_L90)||\
		 (((cmd)&CMD_MASK)==ROP_RECT_COPY_R90)||\
		 (((cmd)&CMD_MASK)==ROP_RECT_COPY_X180)||\
		 (((cmd)&CMD_MASK)==ROP_RECT_COPY_Y180)) {\
	    (out)= ((cmd)&CMD_MASK)|(apa16_rop2stype[rrop&0xf]&STYPE_MASK);\
	}\
	else {\
	    ErrorF("WSGO! Unknown rasterop 0x%x\n",(cmd));\
	}


#define FILL_RECT(cmd,x,y,dx,dy)	\
		CHECK_QUEUE(5,(REG_LOAD(7,x,5),REG_LOAD(6,y,4),\
		 	    REG_LOAD(9,dx,3),REG_LOAD(8,dy,2),\
			    LOAD_QUEUE((cmd),1)))

#define COPY_RECT(cmd,xd,yd,xs,ys,dx,dy)	\
		CHECK_QUEUE(7,(REG_LOAD(7,xd,7),REG_LOAD(0xA,yd,6),\
		 	    REG_LOAD(5,xs,5),REG_LOAD(4,ys,4),\
		 	    REG_LOAD(9,dx,3),REG_LOAD(0,dy,2),\
		 	    LOAD_QUEUE((cmd),1)))

#define DRAW_VECTOR(cmd,fx,fy,tx,ty)	\
		CHECK_QUEUE(5,(REG_LOAD(1,fx,5),REG_LOAD(0,fy,4),\
			    REG_LOAD(5,tx,3),REG_LOAD(6,ty,2),\
			    LOAD_QUEUE((cmd),1)))

#define POLY_VECTOR(cmd,tx,ty) \
		CHECK_QUEUE(3,(REG_LOAD(5,tx,3),REG_LOAD(6,ty,2),\
			    LOAD_QUEUE((cmd),1)))

#define VIDEO_ON() \
		CHECK_QUEUE(1,(LOAD_QUEUE(ROP_VIDEO_ON,1)))

#define VIDEO_OFF() \
		CHECK_QUEUE(1,(LOAD_QUEUE(ROP_VIDEO_OFF,1)))
#endif /* ndef HDWR_SEEN */
