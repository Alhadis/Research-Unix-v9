/*
 *	$Source: /orpheus/u1/X11/NRFPT/puzzle/RCS/pmap.h,v $
 *	$Header: pmap.h,v 1.1 87/09/08 17:28:34 swick Exp $
 */

/*
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; File:         pmap.h
; SCCS:         %A% %G% %U%
; Description:  Glamor Pixel Map (in C) include file
; Author:       Jack Palevich, VGD, ITL, DCC, HP Labs
; Created:      29-Jan-86
; Modified:     15-Mar-86 14:19:55 (Jack Palevich)
; Language:     Text
; Package:      PSL
; Status:       Experimental (Do Not Distribute)
;
; (c) Copyright 1986, Hewlett-Packard Company, all rights reserved.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
*/

#include <stdio.h>
typedef struct {
	short unsigned int w;		/* width */
	short unsigned int h;		/* height */
	unsigned char bpp;		/* bits per pixel 1..32 */
	short unsigned int type;	/* type of pixel (0..7 */
	unsigned int stride;		/* width of a line in bytes */
	char *pixels;			/* pixel map */
	} pixel_map;

typedef struct {
	int left, top, right, bottom;
	} rect;

typedef struct {
	int x, y;
	} point;

typedef enum {
	a_over_b, store
	} composite_op;

extern unsigned char pixel_map_bpp[];	/* bits per pixel */
extern FILE *pixel_map_open();
extern int pixel_map_parse_header();
extern int pixel_map_read();
extern int pixel_map_write();
extern int pixel_map_close();
extern char *pixel_map_alloc();
extern int pixel_map_free();
extern pixel_map *new_pixel_map();
