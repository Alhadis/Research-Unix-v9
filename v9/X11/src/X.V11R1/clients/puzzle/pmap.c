/*
 *	$Source: /orpheus/u1/X11/NRFPT/puzzle/RCS/pmap.c,v $
 *	$Header: pmap.c,v 1.1 87/09/08 17:26:40 swick Exp $
 */

#ifndef lint
static char *rcsid_pmap_c = "$Header: pmap.c,v 1.1 87/09/08 17:26:40 swick Exp $";
#endif	lint

/*
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; File:		pmap.c
; SCCS:         %A% %G% %U%
; Description:  Pixel Map I/O in C
; Author:       Jack Palevich, VGD, ITL, DCC, HP Labs
; Created:      29-Jan-86
; Modified:     7-Jul-86 12:31:59 (Jack Palevich)
; Language:     Text
; Package:      PSL
; Status:       Experimental (Do Not Distribute)
;
; (c) Copyright 1986, Hewlett-Packard Company, all rights reserved.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
*/

#include <stdio.h>
#include <string.h>
#include "pmap.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%  File i/o for pixel maps
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
% "PMAP" file format is an ad-hoc standard developed for storing pixel maps on
%        secondary storage and for transfering pixel maps between C, Pascal programs
%        and the Prism environment.
% 
% A "PMAP" file consists of a 512-byte header block followed by one or more
%              512-byte blocks of data.
%
% The header block consists of the following information:
%
% byte(s)        Information
% =======        ===========
% 0..1           The number of pixels in the X direction (1..65535)
% 2..3           The number of pixels in the Y direction (1..65535)
% 4..7           The number of bytes in the whole data section (1..2^32-1)
% 8..9           The number of bits per pixel (1..32)
% 10..11         The type of pixel (0..6) where:
%                0 is undefined
%                1 is BMM, 1 bit/pixel
%                2 is GSM, 4 bits/pixel
%                3 is GSM', 8 bits/pixel
%                4 is BMC, 8 bits/pixel
%                5 is GSC, 32 bits/pixel
%                6 is GSC', 32 bits/pixel (Gator-c display)
%		 7 is 2 bits per pixel
%		 8 is YF, 16 bits/pixel, stored in files as two planes, Y, then F.
% 
% 12..13         The number of bytes per row of pixels; the stride (1..65535)
% 14..15         The version of encoding used (currently 0) (0..65535)
% 16..19         The ASCII letters PMAP, as a signature. (Check this first!)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

unsigned char pixel_map_bpp[] = { 0, 1, 4, 8, 8, 32, 32, 2, 16};

/*
 * FILE *pixel_map_open(pixel_map, file_name)
 *
 * Determine the dimensions of the pixel map, and fill in the w, h, and type
 * parts of the pixel map structure
 * returns a file descripter if no error, else returns NULL
 * If the file exists, but is not a PMAP file, then we return NULL.
 */

FILE *pixel_map_open(pm, file_name)
	pixel_map *pm;
	char *file_name;
{
	FILE *fp;
	unsigned char buf[512];

	if ((fp = fopen(file_name,"r")) == NULL)
		return(fp);

	if (fread(buf, 1, 512, fp) != 512 ||		/* can we read it? */
		pixel_map_parse_header(pm, buf) == 0) {
		fclose(fp);
		return(NULL);
		}
	return(fp);
	}

/*	isphead.h - declaration for HP-ISP image file header	*/

/*	Ho John Lee, HP Labs	10-6-84				*/

#define SHORT 2
#define LONG 4

typedef struct
	{
	short	filetype;	/* mark what kind of file we have */
	short	bpp;		/* bits per pixel, 1, 4, 8, 24, 32 */
	short	rows;		/* up to 1024 */
	short	cols;		/* up to 1024 */
	short	vectortype;	/* so far, 0 = rectangular blocks */
	short	vectorsize;	/* so far, 16 for 4x4 square blocks */
	long	nvectors;	/* up to 65536 */
	short	brows;		/* number of block rows */
	short	bcols;		/* number of block cols */
	short	vrows;		/* number of vector rows */
	short	vcols;		/* number of vector cols */
	short	packed;		/* format of data */
	char	reserved[128 - 11*SHORT - LONG];
				/* pad variable space to 128 bytes	*/
	char	user[128];	/* user application space		*/
	char	comment[256];	/* rest of 1st block for comments	*/
	} ISP_file_header;

#define FHDRSIZE	sizeof(ISP_file_header)

/*
 * int pixel_map_parse_header(pixel_map, header)
 *
 * Determine the dimensions of the pixel map, and fill in the w, h, and type
 * parts of the pixel map structure.
 * (header is a 512 byte buffer).
 * Reads packed ISP and Glamor PMAP formats.
 * returns 1 if no problem.
 * If the data is not a PMAP or ISP header, we return 0.
 *
 * BUGS: buffer must be word aligned, or else ISP files will generate bus errors.
 */

int pixel_map_parse_header(pm, buf)
	pixel_map *pm;
	unsigned char *buf;
{
	ISP_file_header *isp;

	if (strncmp((char *) buf+16, "PMAP", 4) == 0) {	/* check if it's a PMAP file */
		/* it is a PMAP file. */
		if (buf[14] != 0 || buf[15] != 0) /* check encoding = 0 */
			return(0);

		/* get pixel data */
		pm->w = (buf[0] << 8) | buf[1];
		pm->h = (buf[2] << 8) | buf[3];
		pm->type = (buf[10] << 8) | buf[11];
		pm->stride = (buf[12] << 8) | buf[13];
		pm->bpp = (buf[8] << 8) | buf[9];
		return(1);
		}

	/* check if it's an ISP file */
	isp = (ISP_file_header *) buf;

	if (isp->filetype == 0 &&
		(isp->bpp >= 8 ||
		isp->packed == 1)) { /* it MIGHT be an ISP file.... */
		pm->w = isp->cols;
		pm->h = isp->rows;
		pm->bpp = isp->bpp;
		switch(pm->bpp) {
		case 1: pm->type = 1; break;
		case 2: pm->type = 7; break;
		case 4: pm->type = 2; break;
		case 8: pm->type = 3; break;
		case 32: pm->type = 5; break;
		default:
		 	/* not an ISP file */

			return(0);			
			break;
		}
		pm->stride = (pm->w * pm->bpp) >> 3;
		return(1);
		} /* if */
    	return(0);
}

int pixel_map_read(pm, fp)
	pixel_map *pm;
	FILE *fp;
{
	return pm-> h == fread(pm->pixels, pm->stride, pm->h, fp);
	}

/*
 * pixel_map_write -- returns 1 if successful, else returns 0
 */

int pixel_map_write(pm, fp)
	pixel_map *pm;
	FILE *fp;
{
	char block[512];
	short unsigned int h, w, stride, type, *block_2b;
	unsigned int size, *block_4b;

	block_2b = (short unsigned int *) block;
	block_4b = (unsigned int *) block;

	w = pm->w;
	h = pm->h;
	stride = pm->stride;
	type = pm->type;
	size=h*stride;
	
	block_2b[0] = w;
	block_2b[1] = h;
	block_4b[1] = size;

	block_2b[4] = pixel_map_bpp[type];
	block_2b[5] = type;

	block_2b[6] = stride;
	block_2b[7] = 0;
	block[16] = 'P';
	block[17] = 'M';
	block[18] = 'A';
	block[19] = 'P';

	return (512 == fwrite(block, 1, 512, fp)
		&& size == fwrite(pm->pixels, 1, size, fp));
	}
					  	  
int pixel_map_close(fp)
	FILE *fp;
{
	return fclose(fp);
	}

char *pixel_map_alloc(pm)
	pixel_map *pm;
{
	char *calloc();
	if(pm->stride == 0)
		pm->stride =
			(31 + pm->w * pixel_map_bpp[pm->type]) >> 3;
	return (pm->pixels = calloc(pm->h, pm->stride));
	}

int pixel_map_free(pm)
	pixel_map *pm;
{
	free(pm->pixels);
	free(pm);
	}

pixel_map *new_pixel_map()
{
	pixel_map *joe;
	joe = (pixel_map *) calloc(1, sizeof(pixel_map));
	return(joe);
	}

