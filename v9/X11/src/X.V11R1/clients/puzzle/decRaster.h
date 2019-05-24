/*
 *	$Source: /orpheus/u1/X11/NRFPT/puzzle/RCS/decRaster.h,v $
 *	$Header: decRaster.h,v 1.1 87/09/08 17:28:14 swick Exp $
 */

/*
 * Copyright 1986 by Ed James, UC Berkeley.  All rights reserved.
 *
 * Copy permission is hereby granted provided that this notice is
 * retained on all partial or complete copies.
 *
 * For more info on this and all of my stuff, mail edjames@berkeley.edu.
 */
typedef struct color {
	u_short	red;
	u_short	green;
	u_short	blue;
} COLOR;

typedef struct raster {
	int	magic;
	int	whatever[5];
	int	numColors;		/* size of colormap */
	int	something[5];
	int	height;
	int	width;
	int	yowsa[7];
	char	name1[116];
	char	name2[312];
} RASTERFILE;
