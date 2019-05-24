/******************************************************************
Copyright 1987 by Apollo Computer Inc., Chelmsford, Massachusetts.

                        All Rights Reserved

Permission to use, duplicate, change, and distribute this software and
its documentation for any purpose and without fee is granted, provided
that the above copyright notice appear in such copy and that this
copyright notice appear in all supporting documentation, and that the
names of Apollo Computer Inc. or MIT not be used in advertising or publicity
pertaining to distribution of the software without written prior permission.
******************************************************************/
                    
#include "apollo.h"

Bool apRealizeCursor(), apUnrealizeCursor();
Bool apDisplayCursor(), apRemoveCursor();
Bool apSetCursorPosition();
void apCursorLimits();
void apPointerNonInterestBox();
void apConstrainCursor();
void apQueryBestSize();
void apResolveColor();
void apCreateColormap();
void apDestroyColormap();

CursorPtr       pCurCursor;

static int		lastEventTime;
static int              fdApollo = 0;
static DevicePtr	apKeyboard;
static DevicePtr	apPointer;

static pointer          bitmap_ptr;
static short            words_per_line;
static gpr_$disp_char_t display_char;

static ScreenPtr        pCurScreen;

static int              hotX, hotY;
static BoxRec           constraintBox;

static time_$clock_t    beep_time;

static Bool             controlIsDown = FALSE;
static Bool             shiftIsDown = FALSE;

#define KEY_DISABLED 0
#define KEY_WHITE 1
#define KEY_BLACK 2
#define KEY_PLAIN 0
#define KEY_CTRL 1
#define KEY_SHFT 2
#define KEY_UP 3

static keyRec Keys[256] = {
    KEY_WHITE,    KEY_CTRL,  0x20,  /* 0x00 : ^SP */
    KEY_WHITE,    KEY_CTRL,  0x61,  /* 0x01 : ^A */
    KEY_WHITE,    KEY_CTRL,  0x62,  /* 0x02 : ^B */
    KEY_WHITE,    KEY_CTRL,  0x63,  /* 0x03 : ^C */
    KEY_WHITE,    KEY_CTRL,  0x64,  /* 0x04 : ^D */
    KEY_WHITE,    KEY_CTRL,  0x65,  /* 0x05 : ^E */
    KEY_WHITE,    KEY_CTRL,  0x66,  /* 0x06 : ^F */
    KEY_WHITE,    KEY_CTRL,  0x67,  /* 0x07 : ^G */
    KEY_WHITE,    KEY_CTRL,  0x68,  /* 0x08 : ^H */
    KEY_WHITE,    KEY_CTRL,  0x69,  /* 0x09 : ^I */
    KEY_WHITE,    KEY_CTRL,  0x6A,  /* 0x0A : ^J */
    KEY_WHITE,    KEY_CTRL,  0x6B,  /* 0x0B : ^K */
    KEY_WHITE,    KEY_CTRL,  0x6C,  /* 0x0C : ^L */
    KEY_WHITE,    KEY_CTRL,  0x6D,  /* 0x0D : ^M */
    KEY_WHITE,    KEY_CTRL,  0x6E,  /* 0x0E : ^N */
    KEY_WHITE,    KEY_CTRL,  0x6F,  /* 0x0F : ^O */
    KEY_WHITE,    KEY_CTRL,  0x70,  /* 0x10 : ^P */
    KEY_WHITE,    KEY_CTRL,  0x71,  /* 0x11 : ^Q */
    KEY_WHITE,    KEY_CTRL,  0x72,  /* 0x12 : ^R */
    KEY_WHITE,    KEY_CTRL,  0x73,  /* 0x13 : ^S */
    KEY_WHITE,    KEY_CTRL,  0x74,  /* 0x14 : ^T */
    KEY_WHITE,    KEY_CTRL,  0x75,  /* 0x15 : ^U */
    KEY_WHITE,    KEY_CTRL,  0x76,  /* 0x16 : ^V */
    KEY_WHITE,    KEY_CTRL,  0x77,  /* 0x17 : ^W */
    KEY_WHITE,    KEY_CTRL,  0x78,  /* 0x18 : ^X */
    KEY_WHITE,    KEY_CTRL,  0x79,  /* 0x19 : ^Y */
    KEY_WHITE,    KEY_CTRL,  0x7A,  /* 0x1A : ^Z */
    KEY_WHITE,    KEY_PLAIN, 0x1B,  /* 0x1B : ESC */
    KEY_WHITE,    KEY_CTRL,  0x5C,  /* 0x1C : ^\ */
    KEY_WHITE,    KEY_CTRL,  0x5D,  /* 0x1D : ^] */
    KEY_WHITE,    KEY_CTRL,  0x60,  /* 0x1E : ^` */
    KEY_WHITE,    KEY_CTRL,  0x2F,  /* 0x1F : ^/ */
    KEY_WHITE,    KEY_PLAIN, 0x20,  /* 0x20 : SP */
    KEY_WHITE,    KEY_SHFT,  0x31,  /* 0x21 : ! */
    KEY_WHITE,    KEY_SHFT,  0x27,  /* 0x22 : " */
    KEY_WHITE,    KEY_SHFT,  0x33,  /* 0x23 : # */
    KEY_WHITE,    KEY_SHFT,  0x34,  /* 0x24 : $ */
    KEY_WHITE,    KEY_SHFT,  0x35,  /* 0x25 : % */
    KEY_WHITE,    KEY_SHFT,  0x37,  /* 0x26 : & */
    KEY_WHITE,    KEY_PLAIN, 0x27,  /* 0x27 : ' */
    KEY_WHITE,    KEY_SHFT,  0x39,  /* 0x28 : ( */
    KEY_WHITE,    KEY_SHFT,  0x30,  /* 0x29 : ) */
    KEY_WHITE,    KEY_SHFT,  0x38,  /* 0x2A : * */
    KEY_WHITE,    KEY_SHFT,  0x3D,  /* 0x2B : + */
    KEY_WHITE,    KEY_PLAIN, 0x2C,  /* 0x2C : , */
    KEY_WHITE,    KEY_PLAIN, 0x2D,  /* 0x2D : - */
    KEY_WHITE,    KEY_PLAIN, 0x2E,  /* 0x2E : . */
    KEY_WHITE,    KEY_PLAIN, 0x2F,  /* 0x2F : / */
    KEY_WHITE,    KEY_PLAIN, 0x30,  /* 0x30 : 0 */
    KEY_WHITE,    KEY_PLAIN, 0x31,  /* 0x31 : 1 */
    KEY_WHITE,    KEY_PLAIN, 0x32,  /* 0x32 : 2 */
    KEY_WHITE,    KEY_PLAIN, 0x33,  /* 0x33 : 3 */
    KEY_WHITE,    KEY_PLAIN, 0x34,  /* 0x34 : 4 */
    KEY_WHITE,    KEY_PLAIN, 0x35,  /* 0x35 : 5 */
    KEY_WHITE,    KEY_PLAIN, 0x36,  /* 0x36 : 6 */
    KEY_WHITE,    KEY_PLAIN, 0x37,  /* 0x37 : 7 */
    KEY_WHITE,    KEY_PLAIN, 0x38,  /* 0x38 : 8 */
    KEY_WHITE,    KEY_PLAIN, 0x39,  /* 0x39 : 9 */
    KEY_WHITE,    KEY_SHFT,  0x3B,  /* 0x3A : : */
    KEY_WHITE,    KEY_PLAIN, 0x3B,  /* 0x3B : ; */
    KEY_WHITE,    KEY_SHFT,  0x2C,  /* 0x3C : < */
    KEY_WHITE,    KEY_PLAIN, 0x3D,  /* 0x3D : = */
    KEY_WHITE,    KEY_SHFT,  0x2E,  /* 0x3E : > */
    KEY_WHITE,    KEY_SHFT,  0x2F,  /* 0x3F : ? */
    KEY_WHITE,    KEY_SHFT,  0x32,  /* 0x40 : @ */
    KEY_WHITE,    KEY_SHFT,  0x61,  /* 0x41 : A */
    KEY_WHITE,    KEY_SHFT,  0x62,  /* 0x42 : B */
    KEY_WHITE,    KEY_SHFT,  0x63,  /* 0x43 : C */
    KEY_WHITE,    KEY_SHFT,  0x64,  /* 0x44 : D */
    KEY_WHITE,    KEY_SHFT,  0x65,  /* 0x45 : E */
    KEY_WHITE,    KEY_SHFT,  0x66,  /* 0x46 : F */
    KEY_WHITE,    KEY_SHFT,  0x67,  /* 0x47 : G */
    KEY_WHITE,    KEY_SHFT,  0x68,  /* 0x48 : H */
    KEY_WHITE,    KEY_SHFT,  0x69,  /* 0x49 : I */
    KEY_WHITE,    KEY_SHFT,  0x6A,  /* 0x4A : J */
    KEY_WHITE,    KEY_SHFT,  0x6B,  /* 0x4B : K */
    KEY_WHITE,    KEY_SHFT,  0x6C,  /* 0x4C : L */
    KEY_WHITE,    KEY_SHFT,  0x6D,  /* 0x4D : M */
    KEY_WHITE,    KEY_SHFT,  0x6E,  /* 0x4E : N */
    KEY_WHITE,    KEY_SHFT,  0x6F,  /* 0x4F : O */
    KEY_WHITE,    KEY_SHFT,  0x70,  /* 0x50 : P */
    KEY_WHITE,    KEY_SHFT,  0x71,  /* 0x51 : Q */
    KEY_WHITE,    KEY_SHFT,  0x72,  /* 0x52 : R */
    KEY_WHITE,    KEY_SHFT,  0x73,  /* 0x53 : S */
    KEY_WHITE,    KEY_SHFT,  0x74,  /* 0x54 : T */
    KEY_WHITE,    KEY_SHFT,  0x75,  /* 0x55 : U */
    KEY_WHITE,    KEY_SHFT,  0x76,  /* 0x56 : V */
    KEY_WHITE,    KEY_SHFT,  0x77,  /* 0x57 : W */
    KEY_WHITE,    KEY_SHFT,  0x78,  /* 0x58 : X */
    KEY_WHITE,    KEY_SHFT,  0x79,  /* 0x59 : Y */
    KEY_WHITE,    KEY_SHFT,  0x7A,  /* 0x5A : Z */
    KEY_WHITE,    KEY_PLAIN, 0x5B,  /* 0x5B : [ */
    KEY_WHITE,    KEY_PLAIN, 0x5C,  /* 0x5C : \ */
    KEY_WHITE,    KEY_PLAIN, 0x5D,  /* 0x5D : ] */
    KEY_WHITE,    KEY_SHFT,  0x36,  /* 0x5E : ^ */
    KEY_WHITE,    KEY_SHFT,  0x2D,  /* 0x5F : _ */
    KEY_WHITE,    KEY_PLAIN, 0x60,  /* 0x60 : ` */
    KEY_WHITE,    KEY_PLAIN, 0x61,  /* 0x61 : a */
    KEY_WHITE,    KEY_PLAIN, 0x62,  /* 0x62 : b */
    KEY_WHITE,    KEY_PLAIN, 0x63,  /* 0x63 : c */
    KEY_WHITE,    KEY_PLAIN, 0x64,  /* 0x64 : d */
    KEY_WHITE,    KEY_PLAIN, 0x65,  /* 0x65 : e */
    KEY_WHITE,    KEY_PLAIN, 0x66,  /* 0x66 : f */
    KEY_WHITE,    KEY_PLAIN, 0x67,  /* 0x67 : g */
    KEY_WHITE,    KEY_PLAIN, 0x68,  /* 0x68 : h */
    KEY_WHITE,    KEY_PLAIN, 0x69,  /* 0x69 : i */
    KEY_WHITE,    KEY_PLAIN, 0x6A,  /* 0x6A : j */
    KEY_WHITE,    KEY_PLAIN, 0x6B,  /* 0x6B : k */
    KEY_WHITE,    KEY_PLAIN, 0x6C,  /* 0x6C : l */
    KEY_WHITE,    KEY_PLAIN, 0x6D,  /* 0x6D : m */
    KEY_WHITE,    KEY_PLAIN, 0x6E,  /* 0x6E : n */
    KEY_WHITE,    KEY_PLAIN, 0x6F,  /* 0x6F : o */
    KEY_WHITE,    KEY_PLAIN, 0x70,  /* 0x70 : p */
    KEY_WHITE,    KEY_PLAIN, 0x71,  /* 0x71 : q */
    KEY_WHITE,    KEY_PLAIN, 0x72,  /* 0x72 : r */
    KEY_WHITE,    KEY_PLAIN, 0x73,  /* 0x73 : s */
    KEY_WHITE,    KEY_PLAIN, 0x74,  /* 0x74 : t */
    KEY_WHITE,    KEY_PLAIN, 0x75,  /* 0x75 : u */
    KEY_WHITE,    KEY_PLAIN, 0x76,  /* 0x76 : v */
    KEY_WHITE,    KEY_PLAIN, 0x77,  /* 0x77 : w */
    KEY_WHITE,    KEY_PLAIN, 0x78,  /* 0x78 : x */
    KEY_WHITE,    KEY_PLAIN, 0x79,  /* 0x79 : y */
    KEY_WHITE,    KEY_PLAIN, 0x7A,  /* 0x7A : z */
    KEY_WHITE,    KEY_SHFT,  0x5B,  /* 0x7B : { */
    KEY_WHITE,    KEY_SHFT,  0x5C,  /* 0x7C : | */
    KEY_WHITE,    KEY_SHFT,  0x5D,  /* 0x7D : } */
    KEY_WHITE,    KEY_SHFT,  0x60,  /* 0x7E : ~ */
    KEY_WHITE,    KEY_PLAIN, 0x7F,  /* 0x7F : DEL */
    KEY_DISABLED, KEY_PLAIN, 0x80,  /* 0x80 :    */
    KEY_DISABLED, KEY_PLAIN, 0x81,  /* 0x81 : L1 */
    KEY_DISABLED, KEY_PLAIN, 0x82,  /* 0x82 : L2 */
    KEY_DISABLED, KEY_PLAIN, 0x83,  /* 0x83 : L3 */
    KEY_DISABLED, KEY_PLAIN, 0x84,  /* 0x84 : L4 */
    KEY_DISABLED, KEY_PLAIN, 0x85,  /* 0x85 : L5 */
    KEY_DISABLED, KEY_PLAIN, 0x86,  /* 0x86 : L6 */
    KEY_DISABLED, KEY_PLAIN, 0x87,  /* 0x87 : L7 */
    KEY_BLACK,    KEY_PLAIN, 0x88,  /* 0x88 : L8 */
    KEY_DISABLED, KEY_PLAIN, 0x89,  /* 0x89 : L9 */
    KEY_BLACK,    KEY_PLAIN, 0x8A,  /* 0x8A : LA */
    KEY_DISABLED, KEY_PLAIN, 0x8B,  /* 0x8B : LB */
    KEY_BLACK   , KEY_PLAIN, 0x8C,  /* 0x8C : LC */
    KEY_BLACK   , KEY_PLAIN, 0x8D,  /* 0x8D : LD */
    KEY_BLACK   , KEY_PLAIN, 0x8E,  /* 0x8E : LE */
    KEY_BLACK   , KEY_PLAIN, 0x8F,  /* 0x8F : LF */
    KEY_BLACK,    KEY_PLAIN, 0x90,  /* 0x90 : R1 */
    KEY_DISABLED, KEY_PLAIN, 0x91,  /* 0x91 : R2 */
    KEY_DISABLED, KEY_PLAIN, 0x92,  /* 0x92 : R3 */
    KEY_DISABLED, KEY_PLAIN, 0x93,  /* 0x93 : R4 */
    KEY_BLACK   , KEY_PLAIN, 0x94,  /* 0x94 : R5 */
    KEY_WHITE   , KEY_PLAIN, 0x95,  /* 0x95 : BS */
    KEY_WHITE   , KEY_PLAIN, 0x96,  /* 0x96 : CR */
    KEY_WHITE   , KEY_PLAIN, 0x97,  /* 0x97 : TAB */
    KEY_WHITE   , KEY_SHFT,  0x97,  /* 0x98 : STAB */
    KEY_WHITE   , KEY_CTRL,  0x97,  /* 0x99 : CTAB */
    KEY_DISABLED, KEY_PLAIN, 0x9A,  /* 0x9A :    */
    KEY_DISABLED, KEY_PLAIN, 0x9B,  /* 0x9B :    */
    KEY_DISABLED, KEY_PLAIN, 0x9C,  /* 0x9C :    */
    KEY_DISABLED, KEY_PLAIN, 0x9D,  /* 0x9D :    */
    KEY_DISABLED, KEY_PLAIN, 0x9E,  /* 0x9E :    */
    KEY_DISABLED, KEY_PLAIN, 0x9F,  /* 0x9F :    */
    KEY_DISABLED, KEY_PLAIN, 0xA0,  /* 0xA0 :    */
    KEY_DISABLED, KEY_UP,    0x81,  /* 0xA1 : L1U */
    KEY_DISABLED, KEY_UP,    0x82,  /* 0xA2 : L2U */
    KEY_DISABLED, KEY_UP,    0x83,  /* 0xA3 : L3U */
    KEY_DISABLED, KEY_UP,    0x84,  /* 0xA4 : L4U */
    KEY_DISABLED, KEY_UP,    0x85,  /* 0xA5 : L5U */
    KEY_DISABLED, KEY_UP,    0x86,  /* 0xA6 : L6U */
    KEY_DISABLED, KEY_UP,    0x87,  /* 0xA7 : L7U */
    KEY_BLACK,    KEY_UP,    0x88,  /* 0xA8 : L8U */
    KEY_DISABLED, KEY_UP,    0x89,  /* 0xA9 : L9U */
    KEY_BLACK,    KEY_UP,    0x8A,  /* 0xAA : LAU */
    KEY_DISABLED, KEY_UP,    0x8B,  /* 0xAB : LBU */
    KEY_BLACK,    KEY_UP,    0x8C,  /* 0xAC : LCU */
    KEY_BLACK,    KEY_UP,    0x8D,  /* 0xAD : LDU */
    KEY_BLACK,    KEY_UP,    0x8E,  /* 0xAE : LEU */
    KEY_BLACK,    KEY_UP,    0x8F,  /* 0xAF : LFU */
    KEY_BLACK,    KEY_UP,    0x90,  /* 0xB0 : R1U */
    KEY_DISABLED, KEY_UP,    0x91,  /* 0xB1 : R2U */
    KEY_DISABLED, KEY_UP,    0x92,  /* 0xB2 : R3U */
    KEY_DISABLED, KEY_UP,    0x93,  /* 0xB3 : R4U */
    KEY_DISABLED, KEY_UP,    0x94,  /* 0xB4 : R5U */
    KEY_DISABLED, KEY_SHFT,  0x91,  /* 0xB5 : R2S */
    KEY_DISABLED, KEY_SHFT,  0x92,  /* 0xB6 : R3S */
    KEY_DISABLED, KEY_SHFT,  0x93,  /* 0xB7 : R4S */
    KEY_BLACK,    KEY_SHFT,  0x94,  /* 0xB8 : R5S */
    KEY_DISABLED, KEY_PLAIN, 0xB9,  /* 0xB9 :    */
    KEY_DISABLED, KEY_PLAIN, 0xBA,  /* 0xBA :    */
    KEY_DISABLED, KEY_PLAIN, 0xBB,  /* 0xBB :    */
    KEY_DISABLED, KEY_PLAIN, 0xBC,  /* 0xBC :    */
    KEY_DISABLED, KEY_PLAIN, 0xBD,  /* 0xBD :    */
    KEY_DISABLED, KEY_PLAIN, 0xBE,  /* 0xBE :    */
    KEY_DISABLED, KEY_PLAIN, 0xBF,  /* 0xBF :    */
    KEY_BLACK,    KEY_PLAIN, 0xC0,  /* 0xC0 : F1 */
    KEY_BLACK,    KEY_PLAIN, 0xC1,  /* 0xC1 : F2 */
    KEY_BLACK,    KEY_PLAIN, 0xC2,  /* 0xC2 : F3 */
    KEY_BLACK,    KEY_PLAIN, 0xC3,  /* 0xC3 : F4 */
    KEY_BLACK,    KEY_PLAIN, 0xC4,  /* 0xC4 : F5 */
    KEY_BLACK,    KEY_PLAIN, 0xC5,  /* 0xC5 : F6 */
    KEY_BLACK,    KEY_PLAIN, 0xC6,  /* 0xC6 : F7 */
    KEY_BLACK,    KEY_PLAIN, 0xC7,  /* 0xC7 : F8 */
    KEY_BLACK,    KEY_SHFT,  0x90,  /* 0xC8 : R1S */
    KEY_DISABLED, KEY_SHFT,  0x81,  /* 0xC9 : L1S */
    KEY_DISABLED, KEY_SHFT,  0x82,  /* 0xCA : L2S */
    KEY_DISABLED, KEY_SHFT,  0x83,  /* 0xCB : L3S */
    KEY_DISABLED, KEY_SHFT,  0x84,  /* 0xCC : L4S */
    KEY_DISABLED, KEY_SHFT,  0x85,  /* 0xCD : L5S */
    KEY_DISABLED, KEY_SHFT,  0x86,  /* 0xCE : L6S */
    KEY_DISABLED, KEY_SHFT,  0x87,  /* 0xCF : L7S */
    KEY_BLACK,    KEY_SHFT,  0xC0,  /* 0xD0 : F1S */
    KEY_BLACK,    KEY_SHFT,  0xC1,  /* 0xD1 : F2S */
    KEY_BLACK,    KEY_SHFT,  0xC2,  /* 0xD2 : F3S */
    KEY_BLACK,    KEY_SHFT,  0xC3,  /* 0xD3 : F4S */
    KEY_BLACK,    KEY_SHFT,  0xC4,  /* 0xD4 : F5S */
    KEY_BLACK,    KEY_SHFT,  0xC5,  /* 0xD5 : F6S */
    KEY_BLACK,    KEY_SHFT,  0xC6,  /* 0xD6 : F7S */
    KEY_BLACK,    KEY_SHFT,  0xC7,  /* 0xD7 : F8S */
    KEY_BLACK,    KEY_SHFT,  0x90,  /* 0xD8 : L8S */
    KEY_DISABLED, KEY_SHFT,  0x81,  /* 0xD9 : L9S */
    KEY_BLACK,    KEY_SHFT,  0x82,  /* 0xDA : LAS */
    KEY_DISABLED, KEY_SHFT,  0x83,  /* 0xDB : LBS */
    KEY_BLACK,    KEY_SHFT,  0x84,  /* 0xDC : LCS */
    KEY_BLACK,    KEY_SHFT,  0x85,  /* 0xDD : LDS */
    KEY_BLACK,    KEY_SHFT,  0x86,  /* 0xDE : LES */
    KEY_BLACK,    KEY_SHFT,  0x87,  /* 0xDF : LFS */
    KEY_BLACK,    KEY_UP,    0xC0,  /* 0xE0 : F1U */
    KEY_BLACK,    KEY_UP,    0xC1,  /* 0xE1 : F2U */
    KEY_BLACK,    KEY_UP,    0xC2,  /* 0xE2 : F3U */
    KEY_BLACK,    KEY_UP,    0xC3,  /* 0xE3 : F4U */
    KEY_BLACK,    KEY_UP,    0xC4,  /* 0xE4 : F5U */
    KEY_BLACK,    KEY_UP,    0xC5,  /* 0xE5 : F6U */
    KEY_BLACK,    KEY_UP,    0xC6,  /* 0xE6 : F7U */
    KEY_BLACK,    KEY_UP,    0xC7,  /* 0xE7 : F8U */
    KEY_DISABLED, KEY_PLAIN, 0xE8,  /* 0xE8 : L1A */
    KEY_DISABLED, KEY_PLAIN, 0xE9,  /* 0xE9 : L2A */
    KEY_DISABLED, KEY_PLAIN, 0xEA,  /* 0xEA : L3A */
    KEY_DISABLED, KEY_PLAIN, 0xEB,  /* 0xEB : R6 */
    KEY_DISABLED, KEY_SHFT,  0xE8,  /* 0xEC : L1AS */
    KEY_DISABLED, KEY_SHFT,  0xE9,  /* 0xED : L2AS */
    KEY_DISABLED, KEY_SHFT,  0xEA,  /* 0xEE : L3AS */
    KEY_DISABLED, KEY_SHFT,  0xEB,  /* 0xEF : R6S */
    KEY_BLACK,    KEY_CTRL,  0xC0,  /* 0xF0 : F1C */
    KEY_BLACK,    KEY_CTRL,  0xC1,  /* 0xF1 : F2C */
    KEY_BLACK,    KEY_CTRL,  0xC2,  /* 0xF2 : F3C */
    KEY_BLACK,    KEY_CTRL,  0xC3,  /* 0xF3 : F4C */
    KEY_BLACK,    KEY_CTRL,  0xC4,  /* 0xF4 : F5C */
    KEY_BLACK,    KEY_CTRL,  0xC5,  /* 0xF5 : F6C */
    KEY_BLACK,    KEY_CTRL,  0xC6,  /* 0xF6 : F7C */
    KEY_BLACK,    KEY_CTRL,  0xC7,  /* 0xF7 : F8C */
    KEY_DISABLED, KEY_UP,    0xE8,  /* 0xF8 : L1AU */
    KEY_DISABLED, KEY_UP,    0xE9,  /* 0xF9 : L2AU */
    KEY_DISABLED, KEY_UP,    0xEA,  /* 0xFA : L3AU */
    KEY_DISABLED, KEY_UP,    0xEB,  /* 0xFB : R6U */
    KEY_DISABLED, KEY_PLAIN, 0xFC,  /* 0xFC :    */
    KEY_DISABLED, KEY_PLAIN, 0xFD,  /* 0xFD :    */
    KEY_DISABLED, KEY_PLAIN, 0xFE,  /* 0xFE :    */
    KEY_DISABLED, KEY_PLAIN, 0xFF   /* 0xFF :    */
    };                           


static Bool
apSaveScreen(pScreen, on)
    ScreenPtr pScreen;
    int on;
{
    if (on == SCREEN_SAVER_FORCER)
    {
	lastEventTime = GetTimeInMillis();
	return TRUE;
    }
    else
        return FALSE;
}

Bool
apScreenClose(index, pScreen)
    int index;
    ScreenPtr pScreen;
{
    status_$t status;

    /* This routine frees all of the dynamically allocated space associate
        with a screen. */

    mfbScreenClose(pScreen);

    gpr_$clear((gpr_$pixel_value_t) 0, status);
/*
 *  gpr_$terminate( false, status);
 */

    return (TRUE);
}

Bool
apScreenInit(index, pScreen, argc, argv)
    int index;
    ScreenPtr pScreen;
    int argc;		/* these two may NOT be changed */
    char **argv;
{
    register PixmapPtr pPixmap;
    Bool retval;
    ColormapPtr pColormap;
    short n_entries, disp_len_ret;
    gpr_$pixel_value_t start_index;
    gpr_$offset_t disp;
    gpr_$bitmap_desc_t init_bitmap;
    gpr_$color_vector_t colors;
    gpr_$keyset_t keyset;
    apProcPtrs *procPtr;
    int dpix, dpiy;
    status_$t status;
    short i;

    pCurCursor = NULL;
    pCurScreen = NULL;

    if (!bitmap_ptr)
    {
        gpr_$inq_disp_characteristics( gpr_$borrow, 1, sizeof( display_char),
             display_char, disp_len_ret, status);

        disp.x_size = display_char.x_visible_size;
        disp.y_size = display_char.y_visible_size;

        gpr_$init(gpr_$borrow, 1, disp, 0, init_bitmap, status);

        colors[0] = gpr_$white;
        start_index = 1;
        n_entries = 1;
        gpr_$set_color_map( start_index, n_entries, colors, status);
        gpr_$inq_bitmap_pointer( init_bitmap, bitmap_ptr, words_per_line, status);

        lib_$init_set(keyset, 256);
        for (i=0; i<256; i++)
            if (Keys[i].key_color != KEY_DISABLED) lib_$add_to_set(keyset, 256, i);

        gpr_$enable_input(gpr_$keystroke, keyset, status);
        gpr_$enable_input(gpr_$buttons, keyset, status);
        gpr_$enable_input(gpr_$locator_stop, keyset, status);
        gpr_$enable_input(gpr_$locator_update, keyset, status);

        smd_$set_quit_char((char) KBD_$ABORT, status);
    }

    gpr_$enable_direct_access(status);

    /* It sure is moronic to have to convert metric to English units so that mfbScreenInit can undo it! */
                                                                                         
    dpix = (display_char.x_pixels_per_cm * 254) / 100;
    dpiy = (display_char.y_pixels_per_cm * 254) / 100;
    retval = mfbScreenInit(index, pScreen, bitmap_ptr,
                           display_char.x_visible_size,
			   display_char.y_visible_size, dpix, dpiy);

    wPrivClass = CreateNewResourceClass();

    /* Apollo screens may have large amounts of extra bitmap to the right of the visible
       area, therefore the PixmapBytePad macro in mfbScreenInit gave the wrong value to
       the devKind field of the Pixmap it made for the screen.  So we fix it here. */

    pPixmap = (PixmapPtr)(pScreen->devPrivate);
    pPixmap->devKind = words_per_line << 1;

    pScreen->CloseScreen = apScreenClose;
    pScreen->SaveScreen = apSaveScreen;
    pScreen->RealizeCursor = apRealizeCursor;
    pScreen->UnrealizeCursor = apUnrealizeCursor;
    pScreen->DisplayCursor = apDisplayCursor;
    pScreen->SetCursorPosition = apSetCursorPosition;
    pScreen->CursorLimits = apCursorLimits;
    pScreen->PointerNonInterestBox = apPointerNonInterestBox;
    pScreen->ConstrainCursor = apConstrainCursor;
    pScreen->RecolorCursor = miRecolorCursor;
    pScreen->QueryBestSize = apQueryBestSize;
    pScreen->ResolveColor = apResolveColor;

/* save the original Mfb assigned routines, and put ours in */
    procPtr = &apProcs;
    procPtr->CreateGC = pScreen->CreateGC;
    procPtr->CreateWindow = pScreen->CreateWindow;
    procPtr->ChangeWindowAttributes = pScreen->ChangeWindowAttributes;
    procPtr->GetImage = pScreen->GetImage;
    procPtr->GetSpans = pScreen->GetSpans;

    pScreen->CreateGC = apCreateGC;
    pScreen->CreateWindow = apCreateWindow;
    pScreen->ChangeWindowAttributes = apChangeWindowAttributes;
    pScreen->GetImage = apGetImage;
    pScreen->GetSpans = apGetSpans;

    pScreen->CreateColormap = apCreateColormap;
    pScreen->DestroyColormap = apDestroyColormap;
    CreateColormap(pScreen->defColormap, pScreen,
                   LookupID(pScreen->rootVisual, RT_VISUALID, RC_CORE),
                   &pColormap, AllocNone, 0);
    mfbInstallColormap(pColormap);

    return(retval);
}

int
apMouseProc(pDev, onoff, argc, argv)
    DevicePtr pDev;
    int onoff, argc;
    char *argv[];
{
    int     i;
    BYTE map[4];

    switch (onoff)
    {
	case DEVICE_INIT: 
	    apPointer = pDev;

	    map[1] = 1;
	    map[2] = 2;
	    map[3] = 3;
	    InitPointerDeviceStruct(
		apPointer, map, 3, apGetMotionEvents, apChangePointerControl);

            if (!fdApollo)
                fdApollo = MakeGPRStream();
	    break;
	case DEVICE_ON: 
	    pDev->on = TRUE;
            hotX = hotY = 0;
	    AddEnabledDevice(fdApollo);
	    break;
	case DEVICE_OFF: 
	    pDev->on = FALSE;
	    break;
	case DEVICE_CLOSE: 
	    break;
    }
    return Success;

}

Bool
LegalModifier(key)
    BYTE key;
{
    if ((key == KBD_$LD) || (key == KBD_$LF) || (key == KBD_$R1))
        return TRUE;
    return FALSE;
}

static void
GetApolloMappings(pKeySyms, pModMap)
    KeySymsPtr pKeySyms;
    CARD8 *pModMap;
{
/*
 *  We assume Apollo keyboard number 2 (with mouse, without numeric keypad or lighted
 *  CapsLock key).  (Keyboard number 1 is unsupported, since it can't have a mouse.  Keyboard
 *  number 3 is a superset of keyboard number 2.  It has the ability to generate raw key up/down
 *  transitions; this should be supported but isn't.  Keyboard number 2 cannot generate raw key
 *  up/downs.)
 *
 *  Only the white keys, the four basic arrow keys and F1-F8 are implemented now.
 *  Up transitions for the white keys are faked.
 *  Positions of the real control and shift keys are inferred from the raw input character;
 *      their transitions are faked if necessary.
 *  "Mouse" Control, Shift and Meta keys are as for Apollo V10 driver:
 *      Control:  KBD_$LD    Boxed up-arrow     Lower left corner of left-hand keypad
 *      Shift:    KBD_$LF    Boxed down-arrow   Lower right corner of left-hand keypad
 *      Meta:     KBD_$R1    "POP"              Lower right corner of main keyboard
 *
 *  You can bail out of the server by hitting the ABORT/EXIT key (KBD_$R5S/KBD_$R5).  Unshifted,
 *  it will exit the server in an orderly fashion.  If this doesn't work (i.e. server is wedged),
 *  the shifted version is the system quit character.
 */

/*
 *  MIN_APOLLO_KEY and MAX_APOLLO_KEY are the minimum and maximum unmodified key codes.
 */
#define MIN_APOLLO_KEY 0x1B
#define MAX_APOLLO_KEY 0xC7
#define APOLLO_GLYPHS_PER_KEY 2
#define INDEX(in) ((in - MIN_APOLLO_KEY) * APOLLO_GLYPHS_PER_KEY)

    int i;
    KeySym *map;

   for (i = 0; i < MAP_LENGTH; i++)
        pModMap[i] = NoSymbol;  /* make sure it is restored */
    pModMap[ KBD_$LF ] = ShiftMask;
    pModMap[ KBD_$LD ] = ControlMask;
    pModMap[ KBD_$R1 ] = Mod1Mask;

    map = (KeySym *)Xalloc(sizeof(KeySym) * 
				    (MAP_LENGTH * APOLLO_GLYPHS_PER_KEY));
    pKeySyms->minKeyCode = MIN_APOLLO_KEY;
    pKeySyms->maxKeyCode = MAX_APOLLO_KEY;
    pKeySyms->mapWidth = APOLLO_GLYPHS_PER_KEY;
    pKeySyms->map = map;

    for (i = 0; i < (MAP_LENGTH * APOLLO_GLYPHS_PER_KEY); i++)
	map[i] = NoSymbol;	/* make sure it is restored */

    map[INDEX(KBD_$F1)] = XK_F1;
    map[INDEX(KBD_$F2)] = XK_F2;
    map[INDEX(KBD_$F3)] = XK_F3;
    map[INDEX(KBD_$F4)] = XK_F4;
    map[INDEX(KBD_$F5)] = XK_F5;
    map[INDEX(KBD_$F6)] = XK_F6;
    map[INDEX(KBD_$F7)] = XK_F7;
    map[INDEX(KBD_$F8)] = XK_F8;

    map[INDEX(KBD_$UP_ARROW)] = XK_Up;
    map[INDEX(KBD_$LEFT_ARROW)] = XK_Left;
    map[INDEX(KBD_$RIGHT_ARROW)] = XK_Right;
    map[INDEX(KBD_$DOWN_ARROW)] = XK_Down;

    map[INDEX(KBD_$LD)] = XK_Control_L;
    map[INDEX(KBD_$LF)] = XK_Shift_L;
    map[INDEX(KBD_$R1)] = XK_Meta_L;

    map[INDEX(0x1B)] = XK_Escape;
    map[INDEX(KBD_$TAB)] = XK_Tab;
    map[INDEX(KBD_$BS)] = XK_BackSpace;
    map[INDEX(0x7F)] = XK_Delete;
    map[INDEX(KBD_$CR)] = XK_Return;
    map[INDEX(' ')] = XK_space;

    map[INDEX('1')] = XK_1;
    map[INDEX('1') + 1] = XK_exclam;
    map[INDEX('2')] = XK_2;
    map[INDEX('2') + 1] = XK_at;
    map[INDEX('3')] = XK_3;
    map[INDEX('3') + 1] = XK_numbersign;
    map[INDEX('4')] = XK_4;
    map[INDEX('4') + 1] = XK_dollar;
    map[INDEX('5')] = XK_5;
    map[INDEX('5') + 1] = XK_percent;
    map[INDEX('6')] = XK_6;
    map[INDEX('6') + 1] = XK_asciicircum;
    map[INDEX('7')] = XK_7;
    map[INDEX('7') + 1] = XK_ampersand;
    map[INDEX('8')] = XK_8;
    map[INDEX('8') + 1] = XK_asterisk;
    map[INDEX('9')] = XK_9;
    map[INDEX('9') + 1] = XK_parenleft;
    map[INDEX('0')] = XK_0;
    map[INDEX('0') + 1] = XK_parenright;
    map[INDEX('-')] = XK_minus;
    map[INDEX('-') + 1] = XK_underscore;
    map[INDEX('=')] = XK_equal;
    map[INDEX('=') + 1] = XK_plus;
    map[INDEX('`')] = XK_quoteleft;
    map[INDEX('`') + 1] = XK_asciitilde;

    map[INDEX('q')] = XK_q;
    map[INDEX('q') + 1] = XK_Q;
    map[INDEX('w')] = XK_w;
    map[INDEX('w') + 1] = XK_W;
    map[INDEX('e')] = XK_e;
    map[INDEX('e') + 1] = XK_E;
    map[INDEX('r')] = XK_r;
    map[INDEX('r') + 1] = XK_R;
    map[INDEX('t')] = XK_t;
    map[INDEX('t') + 1] = XK_T;
    map[INDEX('y')] = XK_y;
    map[INDEX('y') + 1] = XK_Y;
    map[INDEX('u')] = XK_u;
    map[INDEX('u') + 1] = XK_U;
    map[INDEX('i')] = XK_i;
    map[INDEX('i') + 1] = XK_I;
    map[INDEX('o')] = XK_o;
    map[INDEX('o') + 1] = XK_O;
    map[INDEX('p')] = XK_p;
    map[INDEX('p') + 1] = XK_P;
    map[INDEX('[')] = XK_bracketleft;
    map[INDEX('[') + 1] = XK_braceleft;
    map[INDEX(']')] = XK_bracketright;
    map[INDEX(']') + 1] = XK_braceright;

    map[INDEX('a')] = XK_a;
    map[INDEX('a') + 1] = XK_A;
    map[INDEX('s')] = XK_s;
    map[INDEX('s') + 1] = XK_S;
    map[INDEX('d')] = XK_d;
    map[INDEX('d') + 1] = XK_D;
    map[INDEX('f')] = XK_f;
    map[INDEX('f') + 1] = XK_F;
    map[INDEX('g')] = XK_g;
    map[INDEX('g') + 1] = XK_G;
    map[INDEX('h')] = XK_h;
    map[INDEX('h') + 1] = XK_H;
    map[INDEX('j')] = XK_j;
    map[INDEX('j') + 1] = XK_J;
    map[INDEX('k')] = XK_k;
    map[INDEX('k') + 1] = XK_K;
    map[INDEX('l')] = XK_l;
    map[INDEX('l') + 1] = XK_L;
    map[INDEX(';')] = XK_semicolon;
    map[INDEX(';') + 1] = XK_colon;
    map[INDEX('\'')] = XK_quoteright;
    map[INDEX('\'') + 1] = XK_quotedbl;
    map[INDEX('\\')] = XK_backslash;
    map[INDEX('\\') + 1] = XK_bar;

    map[INDEX('z')] = XK_z;
    map[INDEX('z') + 1] = XK_Z;
    map[INDEX('x')] = XK_x;
    map[INDEX('x') + 1] = XK_X;
    map[INDEX('c')] = XK_c;
    map[INDEX('c') + 1] = XK_C;
    map[INDEX('v')] = XK_v;
    map[INDEX('v') + 1] = XK_V;
    map[INDEX('b')] = XK_b;
    map[INDEX('b') + 1] = XK_B;
    map[INDEX('n')] = XK_n;
    map[INDEX('n') + 1] = XK_N;
    map[INDEX('m')] = XK_m;
    map[INDEX('m') + 1] = XK_M;
    map[INDEX(',')] = XK_comma;
    map[INDEX(',') + 1] = XK_less;
    map[INDEX('.')] = XK_period;
    map[INDEX('.') + 1] = XK_greater;
    map[INDEX('/')] = XK_slash;
    map[INDEX('/') + 1] = XK_question;
#undef INDEX
}

int
apKeybdProc(pDev, onoff, argc, argv)
    DevicePtr pDev;
    int onoff, argc;
    char *argv[];
{
    int i;
    BYTE map[MAP_LENGTH];
    KeySymsRec keySyms;
    CARD8 modMap[MAP_LENGTH];

    switch (onoff)
    {
	case DEVICE_INIT: 
	    apKeyboard = pDev;

            beep_time.high16 = 0;
            beep_time.low32 = 250*250;         /* 1/4 second */

	    GetApolloMappings (&keySyms, modMap);
	    InitKeyboardDeviceStruct(
		    apKeyboard, &keySyms, modMap, apBell,
		    apChangeKeyboardControl);
	    Xfree(keySyms.map);

            if (!fdApollo)
                fdApollo = MakeGPRStream();
	    break;
	case DEVICE_ON: 
	    pDev->on = TRUE;
	    AddEnabledDevice(fdApollo);
	    break;
	case DEVICE_OFF: 
	    pDev->on = FALSE;
	    break;
	case DEVICE_CLOSE: 
	    break;
    }
    return Success;
}

static int alwaysCheckForInput[2] = {0, 1};

void
cursorUp()
{
    apPrivCurs  *pPriv;
    short       *pshortScr;
    short       *pshortSav;
    long        *plongScr;
    long        *plongSav;
    int         i;

    pPriv = (apPrivCurs *) pCurCursor->devPriv[pCurScreen->myNum];

    if (pPriv->cursorIsDown)
    {
        pPriv->alignment = apEventPosition.x_coord & 0x000F;
        pshortScr = (short *) bitmap_ptr;
        pPriv->pBitsScreen = &pshortScr[(apEventPosition.y_coord*words_per_line) +
                                        (apEventPosition.x_coord >> 4)];
        if (pPriv->alignment)
        {
            plongScr = (long *) pPriv->pBitsScreen;
            plongSav = (long *) pPriv->savedBits;
            for (i=0; i<(pCurCursor->height); i++, plongScr += (words_per_line>>1))
            {
                *plongSav++ = *plongScr;
                *plongScr |= (pPriv->bitsToSet[i]) >> pPriv->alignment;
                *plongScr &= ~((pPriv->bitsToClear[i]) >> pPriv->alignment);
            }
        }
        else    /* must avoid accessing extra words past end of bitmap */
        {
            pshortScr = (short *) pPriv->pBitsScreen;
            pshortSav = (short *) pPriv->savedBits;
            for (i=0; i<(pCurCursor->height); i++, pshortScr += words_per_line)
            {
                *pshortSav++ = *pshortScr;
                *pshortScr |= (pPriv->bitsToSet[i]) >> 16;
                *pshortScr &= (~(pPriv->bitsToClear[i])) >> 16;
            }
        }
    }

    if (pPriv->cursorLeftDown)
        SetInputCheck(apECV, apLastECV);
    pPriv->cursorIsDown = FALSE;
    pPriv->cursorLeftDown = FALSE;

}

static void
cursorDown( willLeaveDown)
    Bool willLeaveDown;
{
    apPrivCurs  *pPriv;
    short       *pshortScr;
    short       *pshortSav;
    long        *plongScr;
    long        *plongSav;
    int         i;

    pPriv = (apPrivCurs *) pCurCursor->devPriv[pCurScreen->myNum];

    if (!(pPriv->cursorIsDown))
    {
        if (pPriv->alignment)
        {
            plongScr = (long *) pPriv->pBitsScreen;
            plongSav = (long *) pPriv->savedBits;
            for (i=0; i<(pCurCursor->height); i++, plongScr += (words_per_line>>1))
                *plongScr = *plongSav++;
        }
        else    /* must avoid accessing extra words past end of bitmap */
        {
            pshortScr = (short *) pPriv->pBitsScreen;
            pshortSav = (short *) pPriv->savedBits;
            for (i=0; i<(pCurCursor->height); i++, pshortScr += words_per_line)
                *pshortScr = *pshortSav++;
        }
    }
    if (willLeaveDown)
        SetInputCheck(&alwaysCheckForInput[0], &alwaysCheckForInput[1]);
    pPriv->cursorIsDown = TRUE;
    pPriv->cursorLeftDown = willLeaveDown;
}

static void
handleButton (xEp)
    xEvent  *xEp;
{
    if (apEventData[0] < 'a')
    {
        xEp->u.u.type = ButtonRelease;
        xEp->u.u.detail = apEventData[0] - ('A' - 1);
    }
    else
    {
        xEp->u.u.type = ButtonPress;
        xEp->u.u.detail = apEventData[0] - ('a' - 1);
    }
    (*apPointer->processInputProc) (xEp, apPointer);
}

static void
handleKey (xEp)
    xEvent  *xEp;
{
    keyRec  kr;

    switch (apEventData[0])
    {
        case KBD_$EXIT:
            KillServerResources();
            exit();
            break;
        case KBD_$LD:
        case KBD_$LDS:
            controlIsDown = TRUE;
            break;
        case KBD_$LDU:
            controlIsDown = FALSE;
            break;
        case KBD_$LF:
        case KBD_$LFS:
            shiftIsDown = TRUE;
            break;
        case KBD_$LFU:
            shiftIsDown = FALSE;
            break;
        default:
            break;
    }

    kr = Keys[apEventData[0]];

    switch (kr.key_mods)
    {
        case KEY_PLAIN:
            xEp->u.u.type = KeyPress;
            xEp->u.u.detail = kr.base_key;
            (*apKeyboard->processInputProc) (xEp, apKeyboard);
            if (kr.key_color == KEY_WHITE)
            {
                xEp->u.u.type = KeyRelease;
                (*apKeyboard->processInputProc) (xEp, apKeyboard);
            }
            break;
        case KEY_CTRL:
            xEp->u.u.type = KeyPress;
            if (!controlIsDown)
            {
                xEp->u.u.detail = KBD_$LD;
                (*apKeyboard->processInputProc) (xEp, apKeyboard);
            }
            xEp->u.u.detail = kr.base_key;
            (*apKeyboard->processInputProc) (xEp, apKeyboard);
            xEp->u.u.type = KeyRelease;
            if (kr.key_color == KEY_WHITE)
            {
                (*apKeyboard->processInputProc) (xEp, apKeyboard);
            }
            if (!controlIsDown)
            {
                xEp->u.u.detail = KBD_$LD;
                (*apKeyboard->processInputProc) (xEp, apKeyboard);
            }
            break;
        case KEY_SHFT:
            xEp->u.u.type = KeyPress;
            if (!shiftIsDown)
            {
                xEp->u.u.detail = KBD_$LF;
                (*apKeyboard->processInputProc) (xEp, apKeyboard);
            }
            xEp->u.u.detail = kr.base_key;
            (*apKeyboard->processInputProc) (xEp, apKeyboard);
            xEp->u.u.type = KeyRelease;
            if (kr.key_color == KEY_WHITE)
            {
                (*apKeyboard->processInputProc) (xEp, apKeyboard);
            }
            if (!shiftIsDown)
            {
                xEp->u.u.detail = KBD_$LF;
                (*apKeyboard->processInputProc) (xEp, apKeyboard);
            }
            break;
        case KEY_UP:
            xEp->u.u.type = KeyRelease;
            xEp->u.u.detail = kr.base_key;
            (*apKeyboard->processInputProc) (xEp, apKeyboard);
            break;
    }

}

/*****************
 * ProcessInputEvents:
 *    processes all the pending input events
 *****************/

extern int screenIsSaved;

void
ProcessInputEvents()
{
    xEvent              x;
    int                 timeNow = 0;
    status_$t           status;

    while (GetGPREvent(TRUE, TRUE))
    {
        if (screenIsSaved == SCREEN_SAVER_ON)
            SaveScreens (SCREEN_SAVER_OFF, ScreenSaverReset);

/*  Since GPR events don't come with time stamps, we have to make up our
 *  own.  We do so by starting at "now" (the time this routine was
 *  called), and adding one millisecond to the time for each subsequent
 *  event.  We take some trouble to avoid time going backwards.
 *  Of course, time stamping at the time of dequeueing is wrong, but ....
 */
        if (!timeNow)
        {
            timeNow = GetTimeInMillis();
            if (timeNow <= lastEventTime) timeNow = lastEventTime + 1;
        }
        else
            timeNow++;

        x.u.keyButtonPointer.time = lastEventTime = timeNow;
        x.u.keyButtonPointer.rootX = apEventPosition.x_coord + hotX;
        x.u.keyButtonPointer.rootY = apEventPosition.y_coord + hotY;

        switch (apEventType)
        {
            case gpr_$locator_stop:
            case gpr_$locator_update:
                if (pCurCursor)
                {
                    cursorDown(FALSE);
                    cursorUp();
                }
                x.u.u.type = MotionNotify;
                (*apPointer->processInputProc) (&x, apPointer);
                break;
            case gpr_$buttons:
                handleButton (&x);
                break;
            case gpr_$keystroke:
                handleKey (&x);
                break;
            default:
                break;
        }
    }

    gpr_$set_cursor_position(apEventPosition, status);
    if (pCurCursor)
        if (((apPrivCurs *)(pCurCursor->devPriv[pCurScreen->myNum]))->cursorLeftDown)
            cursorUp();
}

TimeSinceLastInputEvent()
{
    int timeNow;

    if (lastEventTime == 0)
	lastEventTime = GetTimeInMillis();
    timeNow = GetTimeInMillis();
    return (max(0, (timeNow-lastEventTime)));
}

static void
apChangeKeyboardControl(pDevice, ctrl)
    DevicePtr pDevice;
    KeybdCtrl *ctrl;
{
/*
 *  No keyclick, bell pitch control, LEDs or autorepeat control
 */
    beep_time.high16 = 0;
    beep_time.low32 = 250 * ctrl->bell_duration;
}

static void
apBell(loud, pDevice)
    DevicePtr pDevice;
    int loud;
{
/*
 *  No volume control
 */
    if (loud > 0)
        tone_$time(beep_time);
}

static void
apChangePointerControl(pDevice, ctrl)
    DevicePtr pDevice;
    PtrCtrl   *ctrl;
{
/*
 *  Not implementing mouse threshhold or acceleration factor
 */
}

static int
apGetMotionEvents(buff, start, stop)
    CARD32 start, stop;
    xTimecoord *buff;
{
    return 0;
}

static void
resetPointer (newx, newy)
    int newx;
    int newy;
{
    status_$t   status;

    apEventPosition.x_coord = newx - hotX;
    apEventPosition.y_coord = newy - hotY;
    gpr_$set_cursor_position(apEventPosition, status);
}

static Bool
apSetCursorPosition( pScr, newx, newy, generateEvent)
    ScreenPtr	pScr;
    int		newx;
    int		newy;
    Bool	generateEvent;
{
    xEvent	motion;
 
    if (pCurCursor) cursorDown(FALSE);
    pCurScreen = pScr;
    resetPointer (newx, newy);
    if (pCurCursor) cursorUp();
    if (generateEvent)
    {
      if (*apECV != *apLastECV)
	  ProcessInputEvents();
      motion.u.keyButtonPointer.rootX = newx;
      motion.u.keyButtonPointer.rootY = newy;
      motion.u.keyButtonPointer.time = lastEventTime;
      motion.u.u.type = MotionNotify;
      (*apPointer->processInputProc) (&motion, apPointer);
    }
    return TRUE;
}

Bool
apRemoveCursor( willLeaveDown)
    Bool willLeaveDown;
{
    if (pCurCursor) cursorDown( willLeaveDown);
}

static Bool
apDisplayCursor( pScr, pCurs)
    ScreenPtr	pScr;
    CursorPtr	pCurs;
{
    int         x, y;

    if ((hotX != pCurs->xhot) || (hotY != pCurs->yhot))
    {
	if (pCurCursor) cursorDown(FALSE);
	pCurScreen = pScr;
	pCurCursor = pCurs;
	x = apEventPosition.x_coord + hotX;
	y = apEventPosition.y_coord + hotY;
	hotX = pCurs->xhot;
	hotY = pCurs->yhot;
	resetPointer (x, y);
	apConstrainCursor (pScr, &constraintBox);
	if (pCurCursor) cursorUp();
    }
    return TRUE;
}


static void
apPointerNonInterestBox( pScr, pBox)
    ScreenPtr	pScr;
    BoxPtr	pBox;
{
/*
 *  Here we would inhibit the Apollo pointer device from reporting mouse motion
 *  while it remains within *pBox (with the hotX and hotY coordinates subtracted
 *  from both corners).
 */
}

static void
apConstrainCursor( pScr, pBox)
    ScreenPtr	pScr;
    BoxPtr	pBox;
{
    constraintBox = *pBox;
/*
 *  Here we would make the Apollo pointer device stay within constraintBox
 *  (with the hotX and hotY coordinates subtracted from both corners).
 */
}

/*
 * our software cursor must be completely visible
 */
static void
apCursorLimits( pScr, pCurs, pHotBox, pPhysBox)
    ScreenPtr	pScr;
    CursorPtr	pCurs;
    BoxPtr	pHotBox;
    BoxPtr	pPhysBox;	/* return value */
{
    pPhysBox->x1 = max( pHotBox->x1, pCurs->xhot);
    pPhysBox->y1 = max( pHotBox->y1, pCurs->yhot);
    pPhysBox->x2 = min( pHotBox->x2, display_char.x_visible_size + pCurs->xhot - pCurs->width);
    pPhysBox->y2 = min( pHotBox->y2, display_char.y_visible_size + pCurs->yhot - pCurs->height);
}

static Bool
apRealizeCursor (pScr, pCurs)
    ScreenPtr pScr;
    CursorPtr pCurs;
{
    apPrivCurs  *pPriv;
    unsigned long   *pSrcImg;
    unsigned long   *pSrcMsk;
    unsigned long   *pDstFg;
    unsigned long   *pDstBg;
    unsigned long   widthmask, temp;
    int         i;

    pPriv = (apPrivCurs *) Xalloc (sizeof(apPrivCurs));
    pCurs->devPriv[pScr->myNum] = (pointer) pPriv;

    pPriv->cursorIsDown = TRUE;
    pPriv->cursorLeftDown = TRUE;

    bzero((char *)pPriv->bitsToSet, sizeof(long)*16);
    bzero((char *)pPriv->bitsToClear, sizeof(long)*16);

    pSrcImg = (unsigned long *) pCurs->source;
    pSrcMsk = (unsigned long *) pCurs->mask;
    temp = 32 - pCurs->width;
    widthmask = ~((1 << temp) - 1);

    pDstFg = (pCurs->foreRed) ? pPriv->bitsToSet : pPriv->bitsToClear;
    pDstBg = (pCurs->backRed) ? pPriv->bitsToSet : pPriv->bitsToClear;

    for (i=0; i<(pCurs->height); i++, pSrcImg++, pSrcMsk++)
    {
        *pDstFg++ |= ( (*pSrcImg) & *pSrcMsk) & widthmask;
        *pDstBg++ |= (~(*pSrcImg) & *pSrcMsk) & widthmask; 
    }
    return TRUE;
}

static Bool
apUnrealizeCursor (pScr, pCurs)
    ScreenPtr	pScr;
    CursorPtr	pCurs;
{
    Xfree (pCurs->devPriv[pScr->myNum]);
    return TRUE;
}

/*-
 *-----------------------------------------------------------------------
 * apCursorLoc --
 *	If the current cursor is both on and in the given screen,
 *	fill in the given BoxRec with the extent of the cursor and return
 *	TRUE. If the cursor is either on a different screen or not
 *	currently in the frame buffer, return FALSE.
 *
 * Results:
 *	TRUE or FALSE, as above.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
Bool
apCursorLoc (pScreen, pBox)
    ScreenPtr	  pScreen;  	/* Affected screen */
    BoxRec	  *pBox;	/* Box in which to place the limits */
{

    if (!pCurCursor) {
	/*
	 * Firewall: Might be called when initially putting down the cursor
	 */
	return FALSE;
    }

	pBox->x1 = apEventPosition.x_coord - 15;
	pBox->y1 = apEventPosition.y_coord;
	pBox->x2 = apEventPosition.x_coord + 31;
	pBox->y2 = apEventPosition.y_coord + pCurCursor->height;
	return TRUE;
}

static void
apQueryBestSize(class, pwidth, pheight)
int class;
short *pwidth;
short *pheight;
{
    unsigned width, test;

    switch(class)
    {
      case CursorShape:
	  *pwidth = 16;
	  *pheight = 16;
	  break;
      case TileShape:
      case StippleShape:
	  width = *pwidth;
          if (width > 0) {
	  /* Return the closest power of two not less than what they gave me */
	  test = 0x80000000;
	  /* Find the highest 1 bit in the width given */
	  while(!(test & width))
	     test >>= 1;
	  /* If their number is greater than that, bump up to the next
	   *  power of two */
	  if((test - 1) & width)
	     test <<= 1;
	  *pwidth = test;
	  /* We don't care what height they use */
	  }
	  break;
    }
}

void
apResolveColor(pred, pgreen, pblue, pVisual)
    unsigned short	*pred, *pgreen, *pblue;
    VisualPtr		pVisual;
{
    /* Gets intensity from RGB.  If intensity is >= half, pick white, else
     * pick black.  This may well be more trouble than it's worth. */
    *pred = *pgreen = *pblue = 
        (((39L * *pred +
           50L * *pgreen +
           11L * *pblue) >> 8) >= (((1<<8)-1)*50)) ? ~0 : 0;
}

void
apCreateColormap(pmap)
    ColormapPtr	pmap;
{
    int	red, green, blue, pix;

    /* this is a monochrome colormap, it only has two entries, just fill
     * them in by hand.  If it were a more complex static map, it would be
     * worth writing a for loop or three to initialize it */
    red = green = blue = 0;
    AllocColor(pmap, &red, &green, &blue, &pix, 0);
    red = green = blue = ~0;
    AllocColor(pmap, &red, &green, &blue, &pix, 0);

}

void
apDestroyColormap(pmap)
    ColormapPtr	pmap;
{
}

