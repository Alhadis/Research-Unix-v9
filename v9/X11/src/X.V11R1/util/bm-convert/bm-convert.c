/* 
 * $Locker:  $ 
 */ 
static char	*rcsid = "$Header: bm-convert.c,v 1.1 87/06/24 10:54:58 toddb Exp $";
#include <stdio.h>
#include <strings.h>

#define boolean int

unsigned short *ReadX10BitmapFile (prefix, width, height, x_hot, y_hot)
    char *prefix;         /* RETURN */
    int *width, *height;  /* RETURN */
    int *x_hot, *y_hot;  /* RETURN */
    {
    char variable[81];
    int status, value, i, data_length;
    unsigned short *data;
    FILE *file = stdin;

    *width = *height = -1;
    *x_hot = *y_hot = -1;
    while ((status = fscanf (file, "#define %80s %d\n", variable, &value))==2)
    	{
	if (StringEndsWith (variable, "width"))
	    *width = value;
	else if (StringEndsWith (variable, "height"))
    	    *height = value;
	else if (StringEndsWith (variable, "x_hot"))
	    *x_hot = value;
	else if (StringEndsWith (variable, "y_hot"))
    	    *y_hot = value;
	}

    if (*width <= 0) {
        fprintf (stderr, "No valid width parameter found\n");
	exit (-1);
	}
	
    if (*height <= 0) {
        fprintf (stderr, "No valid height parameter found\n");
	exit (-2);
	}

    data_length = ((*width+15)/16) * *height;
    data = (unsigned short *) malloc (data_length*sizeof(unsigned short));
    
    status = fscanf (file, "static short %80s = { 0x%4hx", variable,
	data);  /* fills in 0th element of data array */
    if ((status != 2) || !StringEndsWith (variable, "bits[]")) {
	fprintf (stderr, "First element of short bits array invalid or not found\n");
	exit (-3);
	}
    
    {
    int prefix_len = strlen(variable) - strlen("_bits[]");
    strncpy (prefix, variable, prefix_len);
    prefix[prefix_len] = '\0';
    }

    for (i=1;i<data_length;i++) {
	/* fill in i'th element of data array */
	status = fscanf (file, ", 0x%4hx", data + i);
	if (status != 1) {
	    fprintf (stderr, "%dth short bits array element is invalid\n", i+1);
	    exit (-4);
	    }
    	}
    return (data);
    }

WriteX11BitmapFile (prefix, width, height, x_hot, y_hot, data)
    char *prefix;
    int width, height, x_hot, y_hot;
    unsigned short *data;
    {
    int i, j;
    int shorts_per_line = (width + 15) / 16;
    int data_length = shorts_per_line * height;
    FILE *file = stdout;
    boolean line_pad = ((width % 16) > 0) && ((width % 16) < 9);

    fprintf (file, "#define %s_width %d\n", prefix, width);
    fprintf (file, "#define %s_height %d\n", prefix, height);
    if (x_hot >= 0)
       fprintf (file, "#define %s_x_hot %d\n", prefix, x_hot);
    if (y_hot >= 0)
       fprintf (file, "#define %s_y_hot %d\n", prefix, y_hot);
    fprintf (file, "static char %s_bits[] = {", prefix);
    for (i = j = 0; i < data_length; i++, j++) {
	unsigned short datum = data[i];
	unsigned char b0 = datum & 0xff;
	unsigned char b1 = (datum & 0xff00) >> 8;
        if (j == 0) fprintf (file, "\n   ");
	else if ((j % 12) == 0) fprintf (file, ",\n   ");
	else fprintf (file, ", ");
	fprintf (file, "0x%02x", b0);
	if (!line_pad || ((i + 1) % shorts_per_line)) {
	    if ((++j % 12) == 0) fprintf (file, ",\n   ");
	    else fprintf (file, ", ");
	    fprintf (file, "0x%02x", b1);
	    }
	}
    fprintf (file, "};\n");
    }

/* StringEndsWith returns TRUE if "s" ends with "suffix", else returns FALSE */
boolean StringEndsWith (s, suffix)
  char *s, *suffix;
  {
  int s_len = strlen (s);
  int suffix_len = strlen (suffix);
  return (strcmp (s + s_len - suffix_len, suffix) == 0);
  }

main() {
    char prefix[80];
    int width, height, x_hot, y_hot;
    unsigned short *data = ReadX10BitmapFile (prefix, &width, &height, &x_hot, &y_hot);
    WriteX11BitmapFile (prefix, width, height, x_hot, y_hot, data);
    }

