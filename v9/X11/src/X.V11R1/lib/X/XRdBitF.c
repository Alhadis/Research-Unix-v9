/* Copyright, 1987, Massachusetts Institute of Technology */

#include "copyright.h"

#include "Xlib.h"
#include "Xutil.h"
#include "Xlibint.h"
#include <stdio.h>
#include <strings.h>

#define MAX_LINE 1000


static cleanup(data, stream)
  char *data;
  FILE *stream;
{
  if (data)
    Xfree(data);
  fclose(stream);
}

int XReadBitmapFile(display, d, filename, width, height, bitmap, x_hot, y_hot)
     Display *display;
     Drawable d;
     char *filename;
     int *width, *height;   /* RETURNED */
     Pixmap *bitmap;        /* RETURNED */
     int *x_hot, *y_hot;    /* RETURNED */
{
  FILE *stream;
  char *data = 0;
  char *ptr;
  char line[MAX_LINE];
  int size, bytes;
  char name_and_type[MAX_LINE];
  char *type;
  int value;
  int version10p;
  int padding;
  int bytes_per_line;
  int ww = 0;
  int hh = 0;
  int hx = -1;
  int hy = -1;
  Pixmap pix;

  if (!(stream = fopen(filename, "r")))
    return(BitmapOpenFailed);

  for (;;) {
    if (!fgets(line, MAX_LINE, stream))
      break;
    if (strlen(line) == MAX_LINE-1) {
      cleanup(data, stream);
      return(BitmapFileInvalid);
    }

    if (sscanf(line, "#define %s %d", name_and_type, &value) == 2) {
      if (!(type = rindex(name_and_type, '_')))
	type = name_and_type;
      else
	type++;
      if (!strcmp("width", type))
	ww=value;
      if (!strcmp("height", type))
	hh=value;
      if (!strcmp("hot", type)) {
	if (type--==name_and_type || type--==name_and_type)
	  continue;
	if (!strcmp("x_hot", type))
	  hx = value;
	if (!strcmp("y_hot", type))
	  hy = value;
      }
      continue;
    }
    
    if (sscanf(line, "static short %s = {", name_and_type) == 1)
      version10p = 1;
    else if (sscanf(line, "static char %s = {", name_and_type) == 1)
      version10p = 0;
    else continue;

    if (!(type = rindex(name_and_type, '_')))
      type = name_and_type;
    else
      type++;
    if (strcmp("bits[]", type))
      continue;
    
    if (!ww || !hh) {
      cleanup(data, stream);
      return(BitmapFileInvalid);
    }

    padding = 0;
    if ((ww % 16) && ((ww % 16) < 9) && version10p)
      padding = 1;

    bytes_per_line = (ww+7)/8 + padding;
    
    size = bytes_per_line * hh;
    data = (char *) Xmalloc( size );
    if (!data) {
      cleanup(data, stream);
      return(BitmapNoMemory);
    }

    if (version10p)
      for (bytes=0, ptr=data; bytes<size; (bytes += 2)) {
	if (fscanf(stream, " 0x%x%*[,}]%*[ \n]", &value) != 1) {
	  cleanup(data, stream);
	  return(BitmapFileInvalid);
	}
	*(ptr++) = value & 0xff;
	if (!padding || ((bytes+2) % bytes_per_line))
	  *(ptr++) = value >> 8;
      }
    else
      for (bytes=0, ptr=data; bytes<size; bytes++, ptr++) {
	if (fscanf(stream, " 0x%x%*[,}]%*[ \n]", &value) != 1) {
	  cleanup(data, stream);
	  return(BitmapFileInvalid);
	}
	*ptr=value;
      }
    
  }

  if (!data) {
    cleanup(data, stream);
    return(BitmapFileInvalid);
  }

  pix = XCreateBitmapFromData(display, d, data, ww, hh);
  if (!pix) {
    cleanup(data, stream);
    return(BitmapNoMemory);
  }
  *bitmap = pix;
  *width = ww;
  *height = hh;

  if (x_hot)
    *x_hot = hx;
  if (y_hot)
    *y_hot = hy;

  cleanup(data, stream);
  return(BitmapSuccess);
}
