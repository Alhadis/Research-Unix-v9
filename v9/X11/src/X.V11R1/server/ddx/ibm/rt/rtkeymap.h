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
#ifndef RT_KEYMAP
#define RT_KEYMAP 1

/* $Header: rtkeymap.h,v 1.3 87/09/13 03:30:38 erik Exp $ */
/* $Source: /u1/X11/server/ddx/ibm/rt/RCS/rtkeymap.h,v $ */

#if !defined(lint) && !defined(LOCORE) && defined(RCS_HDRS)
static char *rcsidrtkeymap = "$Header: rtkeymap.h,v 1.3 87/09/13 03:30:38 erik Exp $";
#endif

#define	RT_GLYPHS_PER_KEY	2
#define	RT_MIN_KEY	7
#define	RT_MAX_KEY	0x84
#define	RT_CONTROL	0x11
#define	RT_LEFT_SHIFT	0x12
#define	RT_LOCK		0x14
#define	RT_ALT_L	0x19
#define RT_ALT_R	0x39
#define	RT_ACTION	0x58
#define	RT_RIGHT_SHIFT	0x59
#define RT_NUM_LOCK	0x76

KeySym rtmap[MAP_LENGTH*RT_GLYPHS_PER_KEY] = {
/* RT Scan Code */	/* X11 Equivalent */	/* Key */
/* 0x07 */		XK_F1,		NoSymbol,	/* f1 */
/* 0x08 */		XK_Escape,	NoSymbol,	/* escape */
/* 0x09 */		0x09,		NoSymbol,	/* no key */
/* 0x0a */		0x0a,		NoSymbol,	/* no key */
/* 0x0b */		0x0b,		NoSymbol,	/* no key */
/* 0x0c */		0x0c,		NoSymbol,	/* no key */
/* 0x0d */		XK_Tab,		NoSymbol,	/* tab/back tab */
/* 0x0e */		XK_quoteleft,	XK_asciitilde,	/* back quote/tilde */
/* 0x0f */		XK_F2,		NoSymbol,	/* f2 */
/* 0x10 */		0x10,		NoSymbol,	/* no key */
/* 0x11 */		XK_Control_L,	NoSymbol,	/* ctrl */
/* 0x12 */		XK_Shift_L,	NoSymbol,	/* left shift */
/* 0x13 */		0x13,		NoSymbol,	/* ??? */
/* 0x14 */		XK_Caps_Lock,	NoSymbol,	/* caps lock */
/* 0x15 */		XK_Q,		NoSymbol,	/* q/Q */
/* 0x16 */		XK_1,		XK_exclam,	/* 1/! */
/* 0x17 */		XK_F3,		NoSymbol,	/* f3 */
/* 0x18 */		0x18,		NoSymbol,	/* no key */
/* 0x19 */		XK_Alt_L,	NoSymbol,	/* left alt */
/* 0x1a */		XK_Z,		NoSymbol,	/* z/Z */
/* 0x1b */		XK_S,		NoSymbol,	/* s/S */
/* 0x1c */		XK_A,		NoSymbol,	/* a/A */
/* 0x1d */		XK_W,		NoSymbol,	/* w/W */
/* 0x1e */		XK_2,		XK_at,		/* 2/@ */
/* 0x1f */		XK_F4,		NoSymbol,	/* f4 */
/* 0x20 */		0x20,		NoSymbol,	/* no key */
/* 0x21 */		XK_C,		NoSymbol,	/* c/C */
/* 0x22 */		XK_X,		NoSymbol,	/* x/X */
/* 0x23 */		XK_D,		NoSymbol,	/* d/D */
/* 0x24 */		XK_E,		NoSymbol,	/* e/E */
/* 0x25 */		XK_4,		XK_dollar,	/* 4/$ */
/* 0x26 */		XK_3,		XK_numbersign,	/* 3/# */
/* 0x27 */		XK_F5,		NoSymbol,	/* f5 */
/* 0x28 */		0x28,		NoSymbol,	/* no key */
/* 0x29 */		XK_space,	NoSymbol,	/* space bar */
/* 0x2a */		XK_V,		NoSymbol,	/* v/V */
/* 0x2b */		XK_F,		NoSymbol,	/* f/F */
/* 0x2c */		XK_T,		NoSymbol,	/* t/T */
/* 0x2d */		XK_R,		NoSymbol,	/* r/R */
/* 0x2e */		XK_5,		XK_percent,	/* 5/% */
/* 0x2f */		XK_F6,		NoSymbol,	/* f6 */
/* 0x30 */		0x30,		NoSymbol,	/* no key */
/* 0x31 */		XK_N,		NoSymbol,	/* n/N */
/* 0x32 */		XK_B,		NoSymbol,	/* b/B */
/* 0x33 */		XK_H,		NoSymbol,	/* h/H */
/* 0x34 */		XK_G,		NoSymbol,	/* g/G */
/* 0x35 */		XK_Y,		NoSymbol,	/* y/Y */
/* 0x36 */		XK_6,		XK_asciicircum,	/* 6/^ */
/* 0x37 */		XK_F7,		NoSymbol,	/* f7 */
/* 0x38 */		0x38,		NoSymbol,	/* no key */
/* 0x39 */		XK_Alt_R,	NoSymbol,	/* right alt */
/* 0x3a */		XK_M,		NoSymbol,	/* m/M */
/* 0x3b */		XK_J,		NoSymbol,	/* j/J */
/* 0x3c */		XK_U,		NoSymbol,	/* u/U */
/* 0x3d */		XK_7,		XK_ampersand,	/* 7/& */
/* 0x3e */		XK_8,		XK_asterisk,	/* 8/* */
/* 0x3f */		XK_F8,		NoSymbol,	/* f8 */
/* 0x40 */		0x40,		NoSymbol,	/* no key */
/* 0x41 */		XK_comma,	XK_less,	/* ,/< */
/* 0x42 */		XK_K,		NoSymbol,	/* k/K */
/* 0x43 */		XK_I,		NoSymbol,	/* i/I */
/* 0x44 */		XK_O,		NoSymbol,	/* o/O */
/* 0x45 */		XK_0,		XK_parenright,	/* 0/) */
/* 0x46 */		XK_9,		XK_parenleft,	/* 9/( */
/* 0x47 */		XK_F9,		NoSymbol,	/* f9 */
/* 0x48 */		0x48,		NoSymbol,	/* no key */
/* 0x49 */		XK_period,	XK_greater,	/* ./> */
/* 0x4a */		XK_slash,	XK_question,	/* //? */
/* 0x4b */		XK_L,		NoSymbol,	/* l/L */
/* 0x4c */		XK_semicolon,	XK_colon,	/* ;/: */
/* 0x4d */		XK_P,		NoSymbol,	/* p/P */
/* 0x4e */		XK_minus,	XK_underscore,	/* -/_ */
/* 0x4f */		XK_F10,		NoSymbol,	/* f10 */
/* 0x50 */		0x50,		NoSymbol,	/* no key */
/* 0x51 */		0x51,		NoSymbol,	/* no key */
/* 0x52 */		XK_quoteright,	XK_quotedbl,	/* '/" */
/* 0x53 */		0x53,		NoSymbol,	/* ??? */
/* 0x54 */		XK_bracketleft,	XK_braceleft,	/* [/{ */
/* 0x55 */		XK_equal,	XK_plus,	/* =/+ */
/* 0x56 */		XK_F11,		NoSymbol,	/* f11 */
/* 0x57 */		XK_Print,	NoSymbol,	/* print screen */
/* 0x58 */		XK_Meta_R,	NoSymbol,	/* action */
/* 0x59 */		XK_Shift_R,	NoSymbol,	/* right shift */
/* 0x5a */		XK_Return,	NoSymbol,	/* enter */
/* 0x5b */		XK_bracketright,XK_braceright,	/* ]/} */
/* 0x5c */		XK_backslash,	XK_bar,	/* \/| */
/* 0x5d */		XK_Pause,	NoSymbol,	/* scroll lock */
/* 0x5e */		XK_F12,		NoSymbol,	/* f12 */
/* 0x5f */		0x5f,		NoSymbol,	/* no key */
/* 0x60 */		XK_Down,	NoSymbol,	/* down arrow */
/* 0x61 */		XK_Left,	NoSymbol,	/* left arrow */
/* 0x62 */		XK_Pause,	NoSymbol,	/* pause */
/* 0x63 */		XK_Up,		NoSymbol,	/* up arrow */
/* 0x64 */		XK_Delete,	NoSymbol,	/* delete */
/* 0x65 */		XK_End,		NoSymbol,	/* end */
/* 0x66 */		XK_BackSpace,	NoSymbol,	/* Backspace */
/* 0x67 */		XK_Insert,	NoSymbol,	/* insert */
/* 0x68 */		0x68,		NoSymbol,	/* no key */
/* 0x69 */		XK_KP_1,	NoSymbol,	/* keypad 1 */
/* 0x6a */		XK_Right,	NoSymbol,	/* right arrow */
/* 0x6b */		XK_KP_4,	NoSymbol,	/* keypad 4 */
/* 0x6c */		XK_KP_7,	NoSymbol,	/* keypad 7 */
/* 0x6d */		XK_Next,	NoSymbol,	/* page down */
/* 0x6e */		XK_Home,	NoSymbol,	/* home */
/* 0x6f */		XK_Prior,	NoSymbol,	/* page up */
/* 0x70 */		XK_KP_0,	NoSymbol,	/* keypad 0 */
/* 0x71 */		XK_KP_Decimal,	NoSymbol,	/* keypad period */
/* 0x72 */		XK_KP_2,	NoSymbol,	/* keypad 2 */
/* 0x73 */		XK_KP_5,	NoSymbol,	/* keypad 5 */
/* 0x74 */		XK_KP_6,	NoSymbol,	/* keypad 6 */
/* 0x75 */		XK_KP_8,	NoSymbol,	/* keypad 8 */
/* 0x76 */		XK_Num_Lock,	NoSymbol,	/* num lock */
/* 0x77 */		XK_KP_Divide,	NoSymbol,	/* keypad / */
/* 0x78 */		0x78,		NoSymbol,	/* no key */
/* 0x79 */		XK_KP_Enter,	NoSymbol,	/* keypad enter */
/* 0x7a */		XK_KP_3,	NoSymbol,	/* keypad 3 */
/* 0x7b */		0x7b,		NoSymbol,	/* no key */
/* 0x7c */		XK_KP_Add,	NoSymbol,	/* keypad plus */
/* 0x7d */		XK_KP_9,	NoSymbol,	/* keypad 9 */
/* 0x7e */		XK_KP_Multiply,	NoSymbol,	/* keypad * */
/* 0x7f */		0x7f,		NoSymbol,	/* no key */
/* 0x80 */		0x80,		NoSymbol,	/* no key */
/* 0x81 */		0x81,		NoSymbol,	/* no key */
/* 0x82 */		0x82,		NoSymbol,	/* no key */
/* 0x83 */		0x83,		NoSymbol,	/* no key */
/* 0x84 */		XK_KP_Subtract,	NoSymbol,	/* keypad minus */
};

#endif /* RT_KEYMAP */
