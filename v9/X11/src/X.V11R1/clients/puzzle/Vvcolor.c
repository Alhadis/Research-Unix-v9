/*
 *	$Source: /orpheus/u1/X11/NRFPT/puzzle/RCS/Vvcolor.c,v $
 *	$Header: Vvcolor.c,v 1.1 87/09/08 17:25:33 swick Exp $
 */

#ifndef lint
static char *rcsid_Vvcolor_c = "$Header: Vvcolor.c,v 1.1 87/09/08 17:25:33 swick Exp $";
#endif	lint

/*
 * Vvcolor.c - color error diffusion.
 *
 * Author: Jack Palevich
 *
 * Cribbed lots of code from Bill Holland
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <fcntl.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "pmap.h"
#include "VEGview.h"


/* Reduce the 32 bpp pixel map to at most 'shades' shades of
 * each color, by using error diffusion.
 * Floyd & Steinberg / "An Adaptive Algorithm for Spatial Greyscale "
 * Proceeding of the S.I.D.  Vol. 17/2 Second Quarter 1976
 *  x x x		A = 7/16 of the error
 *  x p A		B = 1/16 of the error
 *  D C B		C = 5/16 of the error
 *			D = 3/16 of the error
 * pixel_map *pm - source pixel map
 * int red_shades - number of shades of red requested
 * int green_shades - number of shades of green requested
 * int blue_shades - number of shades of blue requested
 * unsigned char **xmap - pointer to where to return pointer to reduced image
 * Color *color_map - color map to fill in with shades we use.
 * int *numColors - number of colors actually used.
 */

reduce_color(pm, red_shades, green_shades, blue_shades,
	xmap, color_map_p, num_colors)
	pixel_map *pm;
	int red_shades, green_shades, blue_shades;
	unsigned char **xmap;
	XColor **color_map_p;
	int *num_colors;
{
	int w, h, scale, h_scale, i, j, x, y, n, m;
	short e, e1, e3, e5, e7, ea, eb;
	short r, g, b;
	unsigned char to_shade[256], *crow;
	short *nrow, *nrow_p;
	int *there;
	float f;
	unsigned char *data;
	unsigned char *cmap_v[4];
	unsigned char ***rgb_to_i;
	int color_index, shades, shadev[4], num_colorsv[4];
	XColor *color_map;

	shadev[3] = red_shades;
	shadev[2] = green_shades;
	shadev[1] = blue_shades;

	w = pm->w;
	h = pm->h;

	nrow_p = (short *) calloc(w+2, sizeof(*nrow));
	nrow = nrow_p + 1; /* vector is -1..w */

	there = (int *) calloc(256, sizeof (*there));

    for ( color_index = 1; color_index <4; color_index++) {	

	shades = shadev[color_index];

	cmap_v[color_index] = (unsigned char *) 
		calloc(shades, sizeof(**cmap_v));

	/* Fill to_shade table */
	if ( shades < 2 ) return(0);
	scale = 255 / (shades - 1);
	for ( i = 0; i < 256; i++) {
		f = i / 255.0;
		n = f * (shades-1) + 0.5;
		m = n * scale;
		if ( m > 255) m = 255;
		to_shade[i] = m;
		there[i] = 0;
		}

	/* Error diffuse */

	for ( y = 0; y < h; y++ ) {
		crow = (unsigned char *) (pm->pixels + (pm->stride * y)
			+ color_index);
		eb = 0; /* Traveling 'B' error term */
		ea = 0; /* Traveling 'A' error term */
		for ( x = 0; x < w; x++ ) {
			n = *crow + nrow[x] + ea;
			m = to_shade[n < 0 ? 0 : (n > 255 ? 255 : n)];
			*crow = m;

			there[m]++; /* keep track of used values */

			e = n - m;
			e1 = e / 16;
			e3 = e1 + e1 + e1;
			e5 = e3 + e1 + e1;
			e7 = e - e5 - e3 - e1;
			nrow[x-1] += e3; /* 'D' term */
			nrow[x] = eb + e5; /* 'C' term and last 'B' term */
			eb = e1; /* new 'B' term */
			ea = e7; /* new 'A' term */
			crow += 4; /* Next pixel */
			} /* each pixel */
		} /* each row */

	/* Find out how many colors we actually used */

	i = 0;
	for ( j = 0; j < 256; j++ ) {
		if ( there[j] ) {
			/* A new color */
			there[j] = i;
			cmap_v[color_index][i] = j;
			i++;
			}
		}

	num_colorsv[color_index] = i;

	for ( j = 0; j < pm->h; j++) {
		data = (unsigned char *) pm->pixels + pm->stride * j
			+ color_index;
		for (i = 0; i < pm->w; i++) {
			*data = there[*data];
			data += 4;
			}
		}

	} /* for each color */

	free(nrow_p);
	free(there);

	red_shades = num_colorsv[3];
	green_shades = num_colorsv[2];
	blue_shades = num_colorsv[1];

	i = red_shades * green_shades * blue_shades;
	*num_colors = i;
	if ( i > 256) {
		fprintf(stderr, "This would need a total of %d colors.\n", i);
		}

	/* Fill in the color map and the translation tables */


	color_map = (XColor *) calloc( i, sizeof(XColor));

	*color_map_p = color_map;

	rgb_to_i = (unsigned char ***) calloc(red_shades, sizeof(*rgb_to_i));
	i = 0;
	for ( r = 0; r < red_shades; r++) {
	    rgb_to_i[r] = (unsigned char **)
		calloc(green_shades, sizeof(**rgb_to_i));
	    for ( g = 0; g < green_shades; g++) {
		rgb_to_i[r][g] = (unsigned char *)
			calloc(blue_shades, sizeof(***rgb_to_i));
		for ( b = 0; b < blue_shades; b++) {
			color_map[i].red = cmap_v[3][r] << 8;
			color_map[i].green = cmap_v[2][g] << 8;
			color_map[i].blue = cmap_v[1][b] << 8;
			rgb_to_i[r][g][b] = i;
			i++;
			}
		}
	    }
	/* Convert from three-value rgb to index */
	/* This is destructive to the original data... */

	data = (unsigned char *) pm->pixels;

	for ( y = 0; y < h; y++ ) {
		crow = (unsigned char *) (pm->pixels + (pm->stride * y));
		for ( x = 0; x < w; x++ ) {
			*data = rgb_to_i[crow[3]][crow[2]][crow[1]];
			data++;
			crow += 4;
			}
		}

	for ( r = 0; r < red_shades; r++) {
	    for ( g = 0; g < green_shades; g++) {
		free(rgb_to_i[r][g]);
		}
	    free(rgb_to_i[r]);
	    }
	free(rgb_to_i);

	for ( i = 1; i < 4; i++) {
		free(cmap_v[i]);
		}

	*xmap = (unsigned char *) pm->pixels;

	}
