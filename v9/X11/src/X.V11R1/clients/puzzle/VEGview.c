/*
 *	$Source: /orpheus/u1/X11/NRFPT/puzzle/RCS/VEGview.c,v $
 *	$Header: VEGview.c,v 1.1 87/09/08 17:25:06 swick Exp $
 */

#ifndef lint
static char *rcsid_VEGview_c = "$Header: VEGview.c,v 1.1 87/09/08 17:25:06 swick Exp $";
#endif	lint


#define DEBUG

/*
 * VEGview (c) 1987 Hewlett-Packard Labs.
 *
 * Author: Jack Palevich
 *
 * View Video Enhanced Graphics images on X-Windows
 * Inspired by Ed Moy's Xdisp program
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <fcntl.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "pmap.h"
#include "VEGview.h"

#define PM_DISPLAY	0
#define PM_CONVERT	1
#define IFPMD if(program_mode == PM_DISPLAY)
#define IFPMC if(program_mode == PM_CONVERT)

extern Display 	*dpy;
extern int	screen;
extern GC	gc;
extern Colormap PuzzleColormap;

Pixmap VEGsetup(RasterFile,width,height)
char *RasterFile;
int *width, *height;
{

    int shades, red_shades, green_shades, blue_shades;
    int i,j;
    int numColors;
    unsigned char *xmap, *data;
    XColor *colormap;
    pixel_map *pm;
    Pixmap PicturePixmap;

    read_raster_file(&pm,RasterFile);
   
    switch ((*pm).type) {
      case 3: shades = 16;
	      reduce_grey(pm, shades, &xmap, &colormap, &numColors);
	      break;
      case 6: red_shades = 3;
	      green_shades = 4;
	      blue_shades = 2;
	      reduce_color(pm, red_shades, green_shades, blue_shades, 
		     &xmap, &colormap, &numColors);
	      break;
      default:fprintf(stderr, "Can't interpret this type of PMAP.\n");
	      exit(1);
	      break;
    }

    *width  = (*pm).w;
    *height = (*pm).h;

    if (get_ro_colors(colormap, numColors))
	exit(1);

   
    /* Convert from virtual color_map to actual color_map */

    for ( j = 0; j < pm->h; j++) {
	data = xmap + pm->w * j;
	for (i = 0; i < pm->w; i++)
	    data[i] = colormap[data[i]].pixel;
    }

    /* Try to cache the data as a Pixmap on the server */

    {
	XImage image;

	image.width = pm->w;
	image.height = pm->h;
	image.xoffset = 0;
	image.format = ZPixmap;
	image.data = (char *) xmap;
	image.byte_order = MSBFirst;
	image.bitmap_unit = 8;
	image.depth = 8;
	image.bytes_per_line = pm->w;
	image.bits_per_pixel = 8;

	PicturePixmap = XCreatePixmap(dpy,RootWindow(dpy,screen),
				      image.width,image.height,8);
	XPutImage(dpy,PicturePixmap,gc,&image,0,0,0,0,
		  image.width,image.height);
    }
    return(PicturePixmap);
}   

/* Reduce the 8 bpp pixel map to at most 'shades' shades of
 * grey, by using error diffusion.
 * Floyd & Steinberg / "An Adaptive Algorithm for Spatial Greyscale "
 * Proceeding of the S.I.D.  Vol. 17/2 Second Quarter 1976
 *  x x x		A = 7/16 of the error
 *  x p A		B = 1/16 of the error
 *  D C B		C = 5/16 of the error
 *			D = 3/16 of the error
 * pixel_map *pm - source pixel map
 * int shades - number of shades of grey requested
 * unsigned char **xmap - pointer to where to return pointer to reduced image
 * Color *color_map - color map to fill in with shades we use.
 * int *numColors - number of colors actually used.
 */

reduce_grey(pm, shades, xmap, color_map_p, num_colors)
pixel_map *pm;
int shades;
unsigned char **xmap;
XColor **color_map_p;
int *num_colors;
{
    int w, h, scale, h_scale, i, j, x, y, n, m;
    short e, e1, e3, e5, e7, ea, eb;
    unsigned char to_shade[256], *crow;
    short *nrow, *nrow_p;
    int *there;
    float f;
    unsigned char *data;
    unsigned char *vgrey;
    XColor *color_map;

    w = pm->w;
    h = pm->h;
    nrow_p = (short *) calloc(w+2, sizeof(*nrow));
    nrow = nrow_p + 1; /* vector is -1..w */

    /* Fill to_shade table */
    if ( shades < 2 ) return(0);
    scale = 255 / (shades - 1);
    for ( i = 0; i < 256; i++) {
	f = i / 255.0;
	n = f * (shades-1) + 0.5;
	m = n * scale;
	if ( m > 255) m = 255;
	to_shade[i] = m;
    }

    /* Error diffuse */

    for ( y = 0; y < h; y++ ) {
	crow = (unsigned char *) (pm->pixels + (pm->stride * y));
	eb = 0; /* Traveling 'B' error term */
	ea = 0; /* Traveling 'A' error term */
	for ( x = 0; x < w; x++ ) {
	    n = crow[x] + nrow[x] + ea;
	    m = to_shade[n < 0 ? 0 : (n > 255 ? 255 : n)];
	    crow[x] = m;
	    e = n - m;
	    e1 = e / 16;
	    e3 = e1 + e1 + e1;
	    e5 = e3 + e1 + e1;
	    e7 = e - e5 - e3 - e1;
	    nrow[x-1] += e3; /* 'D' term */
	    nrow[x] = eb + e5; /* 'C' term and last 'B' term */
	    eb = e1; /* new 'B' term */
	    ea = e7; /* new 'A' term */
	} /* each pixel */
    } /* each row */
    free(nrow_p);
    
    /* Find out how many colors we actually used */

    there = (int *) calloc(256, sizeof (*there));

    for ( j = 0; j < pm->h; j++) {
	data = (unsigned char *) pm->pixels + pm->stride * j;
	for (i = 0; i < pm->w; i++) {
	    there[data[i]]++;
	}
    }

    /* Convert to a virtual color map */

    vgrey = (unsigned char *) calloc(shades, sizeof(*vgrey));

    i = 0;
    for ( j = 0; j < 256; j++ ) {
	if ( there[j] ) {
	    /* A new color */
	    there[j] = i;
	    vgrey[i] = j;
	    i++;
	}
    }

    color_map = (XColor *) calloc(i, sizeof(XColor));

    *color_map_p = color_map;

    *num_colors = i;

    for ( j = 0; j < i; j++) {
	color_map[j].red =
	    color_map[j].green =
		color_map[j].blue =
		    vgrey[j] << 8;
    }

    free(vgrey);

    for ( j = 0; j < pm->h; j++) {
	data = (unsigned char *) pm->pixels + pm->stride * j;
	for (i = 0; i < pm->w; i++) {
	    data[i] = there[data[i]];
	}
    }

    free(there);

    *xmap = (unsigned char *) pm->pixels;

}


read_raster_file(ppm, name)
pixel_map **ppm;
char *name;
{
    pixel_map *pm;
    FILE *is;

    pm = new_pixel_map();
    *ppm = pm;

    if (NULL == (is = pixel_map_open(pm, name))) {
	fprintf(stderr, "Couldn't open %s\n", name);
	return(NULL);
    }
    if (NULL == pixel_map_alloc(pm)) {
	fprintf(stderr, "Couldn't allocate storage for pixel map\n");
	fclose(is);
	return(NULL);
    }
    if (0 == pixel_map_read(pm, is)) {
	fprintf(stderr, "Couldn't read pixel map\n");
	fclose(is);
	return(NULL);
    }
    fclose(is);
    
    if ( pm->type == YF_TYPE) {
	pm->type = 3;
	pm->stride = pm->stride / 2;
    }
}

get_ro_colors(color_map, numColors)
XColor *color_map;
int numColors;
{
    int i;
    XColor color;

    for (i = 0; i < numColors; i++) {
	/* Need a read-only color for this value */
	if (!XAllocColor(dpy,PuzzleColormap,&color_map[i])) {
	    fprintf(stderr, "not enough colors (asked for %d, got %d).\n",
		    numColors, i);
	    return (-1);
	}
#ifdef DEBUG
	else
	    printf("color %d pixel value is %d\n",i,color_map[i].pixel);
#endif DEBUG
    }
    return(0);
}
