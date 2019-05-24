/*----------------------------------------------------------------------*/
/*									*/
/*	PACMAN for BBN BitGraphs					*/
/*									*/
/*	          File: blt.c68						*/
/*	      Contents:	block transfer routines				*/
/*	        Author: Bob Brown (rlb)					*/
/*			Purdue CS					*/
/*		  Date: May, 1982					*/
/*	   Description:	see below, customized for pacman		*/
/*									*/
/*----------------------------------------------------------------------*/

#include "style.h"
#include "pacman.h"

/*
** block transfer routines - customized for pacman
**
** blt24 - copies a 24x24 font character to the screen
** blt40 - copies a 40x40 font character to the screen
**
** opcode:
**	REPLACE ... replace mode
**	PAINT ..... or in
**	INVERT .... xor in
*/

extern Bitmap Bitmap24,Bitmap40;

blt24(chr, row, col, opcode)
char chr;
int row,col;
int opcode;
{
	bitblt(&Bitmap24,Rect(24*(chr-'a'),0,24*(chr-'a'+1),24),&display,Pt(col*8+8,row*8+60),(opcode==REPLACE)?F_STORE:F_XOR);
}
/*
** Forty-bit font blt.  
*/
blt40(chr, row, col, opcode)
char chr;
int row,col;
int opcode;
{
	bitblt(&Bitmap40,Rect(40*(chr-'a'),0,40*(chr-'a'+1),40),&display,Pt(col*8,row*8+52),F_XOR);
}

