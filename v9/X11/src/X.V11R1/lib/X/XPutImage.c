#include "copyright.h"

/* $Header: XPutImage.c,v 11.34 87/09/11 15:46:16 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include <stdio.h>
#include "Xlibint.h"
#include <errno.h>


#define ROUNDUP(nbytes, pad) (((((nbytes) - 1) + (pad)) / (pad)) * (pad))

/* XXX Note to who ever must maintain this code:

The helper routine "PutImageRequest" is unfinished and still has a few bugs.
Byte swapping is never required when
    (image->byte_order == image->bitmap_bit_order) &&
    (dpy->byte_order == dpy->bitmap_bit_order)
Someday we will write a treatise on the subject.  If you don't believe us, try it.
Note that all display hardware we know of works this way.
Note that in X10, shipping from a VAX to a SUN ended up byte-swapping twice (once
in dix and once in ddx).  Shipping from a SUN to a VAX byte-swapped once, but
that worked because bitmaps were described as shorts in VAX order, and thus when
compiled on a SUN resulted in the additional byte-swap.

Swapping in the other cases is probably worked out, but we don't have time to
make sure, and we don't have any machines that do things that way to test it on.
You probably need to swap halves within longs without swapping bytes within shorts
for some cases.

Also, if a client sends ZPixmap format images of 4 bits_per_pixel and nibble swapping
is required, this routine does not swap the 4 bit nibbles as it should.

XXX */

static PutImageRequest(dpy, d, gc, image, req_xoffset,req_yoffset, x, y, req_width,
								req_height)
    register Display *dpy;
    Drawable d;
    GC gc;
    register XImage *image;
    int x, y;
    unsigned int req_width, req_height;
    int req_xoffset, req_yoffset;
{
	register xPutImageReq *req;
	int total_xoffset;

	FlushGC(dpy, gc);
	GetReq (PutImage, req);
	/*
	 * first set up the standard stuff in the request
	 */
	req->drawable = d;
	req->gc = gc->gid;
	req->dstX = x;
	req->dstY = y;
	req->width = req_width;
	req->height = req_height;
	req->format = image->format;
	req->depth = image->depth;
        total_xoffset = image->xoffset + req_xoffset;
	switch (image->format) {
	      long length;
	      int i;
	      case XYBitmap:
		/*
		 * If the client side format matches the screen, we win!
		 */
		if ((dpy->bitmap_unit == image->bitmap_unit) 
		    &&(dpy->bitmap_pad == image->bitmap_pad)
		    &&(dpy->byte_order == image->byte_order)
		    &&(dpy->bitmap_bit_order == image->bitmap_bit_order)
		    &&(image->width == req_width)
		    &&(total_xoffset < dpy->bitmap_pad)
		    &&(req_yoffset == 0)) {
		      length = image->bytes_per_line * req_height;
		      req->length += length >> 2;
		      req->leftPad = total_xoffset;
		      _XSend (dpy, image->data, length);
		  }
		else {
		       long nbytes;
		       char *row_addr;
		       register char *p;
		       char* tempbuf;
		       /*
			* compute length of image data; round up to next length
			*/
		       req->leftPad = total_xoffset%dpy->bitmap_pad;
		       nbytes = length = ROUNDUP(req_width + req->leftPad,
						 dpy->bitmap_pad) >> 3;
		       length *= req_height;
		       req->length += length >> 2;

			/*Get space to put the image*/

		       tempbuf = p = _XAllocScratch (dpy, 
				(unsigned long)(length));
			
		       /*Figure out where to start*/

		       row_addr = (image->data)+
			      (image->bytes_per_line*req_yoffset) +
			      ((total_xoffset - req->leftPad)>>3);

		       /* Now copy the desired (sub)image into the buffer */

		       for (i = 0; i < req_height; 
			    	i++, row_addr += image->bytes_per_line,p+=nbytes) {
			   bcopy (row_addr, p, (int)nbytes);
			   if (image->bitmap_bit_order
			     != dpy->bitmap_bit_order) _swapbits (
				(unsigned char *)p, image->bytes_per_line);
#ifdef notdef
			   if ((image->byte_order != image->bitmap_bit_order) ||
			       (dpy->byte_order != dpy->bitmap_bit_order))
			   {
			     /* XXX swapping missing XXX */
			   }
#endif
			}
		        _XSend(dpy,tempbuf,length);
	       }
		  
		break;

	      case XYPixmap:	/* not yet implemented */
		/*
		 * If the client side format matches the screen, we win!
		 */
		if ((dpy->bitmap_unit == image->bitmap_unit) 
		    &&(dpy->bitmap_pad == image->bitmap_pad)
		    &&(dpy->byte_order == image->byte_order)
		    &&(dpy->bitmap_bit_order == image->bitmap_bit_order)
		    &&(image->width == req_width)
		    &&(total_xoffset < dpy->bitmap_pad)
		    &&(req_yoffset == 0)) {
		      length = image->bytes_per_line * req_height * 
								image->depth;
		      req->length += length >> 2;
		      req->leftPad = total_xoffset;
		      _XSend (dpy, image->data, length);
		  }
		else {
		       long nbytes;
		       char *row_addr;
		       register char *p;
		       char* tempbuf;
		       int bytes_per_plane,j;
		       /*
			* compute length of image data; round up to next length
			*/
		       req->leftPad = total_xoffset%dpy->bitmap_pad;
		       nbytes = length = ROUNDUP(req_width + req->leftPad,
						 dpy->bitmap_pad) >> 3;
		       length *= req_height * image->depth;
		       req->length += length >> 2;
		       bytes_per_plane = image->bytes_per_line * image->height;

			/*Get space to put the image*/

		       tempbuf = p = _XAllocScratch (dpy, 
				(unsigned long)(length));

		       /* Loop on planes */
			
		       for (j = 0; j < image->depth; j++) {

			   /*Figure out where to start in image structure*/

			   row_addr = (image->data)+
				  (j * bytes_per_plane) +
				  (image->bytes_per_line*req_yoffset) +
				  ((total_xoffset - req->leftPad)>>3);

			   /* Now copy a plane of the desired (sub)image into the
			      buffer */

			   for (i = 0; i < req_height; 
				    i++, row_addr += image->bytes_per_line,p+=nbytes) {
			       bcopy (row_addr, p, (int)nbytes);
			       if (image->bitmap_bit_order
				 != dpy->bitmap_bit_order) _swapbits (
				    (unsigned char *)p, nbytes);
#ifdef notdef
			       if ((image->byte_order != image->bitmap_bit_order) ||
				   (dpy->byte_order != dpy->bitmap_bit_order))
			       {
				 /* XXX swapping missing XXX */
			       }
#endif
			    }
			}
		        _XSend(dpy,tempbuf,length);
	       }
		  
		break;

	      case ZPixmap:
	        /*
		 * If the client side format matches the screen, we win!
		 */
		/* XXX is this right? */
		req->leftPad = 0;

		if ((dpy->byte_order == image->byte_order)
		   &&(dpy->bitmap_pad == image->bitmap_pad)) {
		    length = image->bytes_per_line * image->height;
		    req->length += length >> 2;
		    _XSend (dpy, image->data, length);
		  }
		else {
		    /*
		     * since we have no other way to go, we presume format
		     * of image matches format of drawable....  Sure makes
		     * things simpler.
		     */
		    long nbytes;
		    unsigned char *row = (unsigned char *) (image->data + req_yoffset * image->bytes_per_line);
		    unsigned char *tempbuf, *p;
		    int bit_xoffset, pseudoLeftPad, byte_xoffset;
		    int j;

		    nbytes = length = ROUNDUP(req_width * image->bits_per_pixel, dpy->bitmap_pad) >> 3;
    
		    tempbuf = p = (unsigned char *)_XAllocScratch(dpy, (unsigned long)nbytes * req_height);

		    length *= req_height;
		    req->length += length >> 2;

		    bit_xoffset = req_xoffset * image->bits_per_pixel;
		    pseudoLeftPad = bit_xoffset % 8;
		    byte_xoffset = bit_xoffset >> 3;

		    for (i = 0; i < req_height; 
				    i++, row += image->bytes_per_line, p+=nbytes) {
			/*
			 * Move the data somewhere we can reformat it.
			 */
		     

		    if (pseudoLeftPad != 0)
		    {
			for ( j=0; j < nbytes; j++)
			{
			    p[j] = (row[byte_xoffset + j] >> (pseudoLeftPad)) | (row[byte_xoffset + j + 1] << ( 8 - pseudoLeftPad));
			}
		
		    }
		    else
			bcopy (row + ((req_xoffset * image->bits_per_pixel) >> 3), p, (int)nbytes);
			if (image->byte_order != dpy->byte_order) {
			  if(image->bits_per_pixel == 32) _swaplong(p, nbytes);
			  else if (image->bits_per_pixel == 24)
			      _swapthree (p, nbytes);
			  else if (image->bits_per_pixel == 16) 
			      _swapshort (p, nbytes);
			}
		      }
		    _XSend(dpy, tempbuf, length);
		}
	        break;
	      default:	/* should never get here */
		(void) fputs ("unknown image type in XPutImage\n", stderr);
		exit(1);
		break;
	}

	SyncHandle();
	return;
}
	

char _reverse_byte[0x100] = {
	0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
	0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
	0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
	0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
	0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
	0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
	0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
	0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
	0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
	0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
	0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
	0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
	0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
	0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
	0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
	0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
	0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
	0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
	0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
	0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
	0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
	0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
	0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
	0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
	0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
	0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
	0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
	0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
	0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
	0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
	0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
	0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};


_swapbits (b, n)
	register unsigned char *b;
	register long n;
{
	do {
		*b = _reverse_byte[*b];
		b++;
	    } while (--n > 0);
	
}

_swapshort (bp, n)
     register char *bp;
     register long n;
{
	register char c;
	register char *ep = bp + n;
	do {
		c = *bp;
		*bp = *(bp + 1);
		bp++;
		*bp = c;
		bp++;
	}
	while (bp < ep);
}

_swaplong (bp, n)
     register char *bp;
     register long n;
{
	register char c;
	register char *ep = bp + n;
	register char *sp;
	do {
	  	sp = bp + 3;
		c = *sp;
		*sp = *bp;
		*bp++ = c;
		sp = bp + 1;
		c = *sp;
		*sp = *bp;
		*bp++ = c;
		bp += 2;
	}
	while (bp < ep);
}

_swapthree (bp, n)
     register char *bp;
     register long n;
{
	register char c;
	register char *ep = bp + n;
	do {
	  	c = *bp;
		*(bp + 2) = *bp;
		*bp = c;
		bp += 3;
	}
	while (bp < ep);
}


static int
BytesPerImageRow(dpy, image, req_width, req_xoffset)
    Display *dpy;
    XImage *image;
    unsigned int req_width;
    int req_xoffset;
{
    int total_offset = image->xoffset + req_xoffset;
    int left_pad = total_offset % dpy->bitmap_pad;
    int nbytes;

    switch (image->format) 
    {
	case XYBitmap:
            nbytes = ROUNDUP(req_width + left_pad, dpy->bitmap_pad) >> 3;
   	    break;

        case XYPixmap:
	    nbytes = (ROUNDUP(req_width + left_pad, dpy->bitmap_pad) >> 3)
		* image->depth;
           break;
	case ZPixmap:
            nbytes = ROUNDUP(req_width * image->bits_per_pixel, 
		dpy->bitmap_pad) >> 3;
 	   break;
	default:	/* should never get here */
	    (void) fputs ("unknown image type in XPutImage\n", stderr);
	    exit(1);
	    break;
    }
 
    return nbytes;
}
	   
static void
PutSubImage (dpy, d, gc, image, req_xoffset,req_yoffset, x, y, req_width,
								req_height)
    register Display *dpy;
    Drawable d;
    GC gc;
    register XImage *image;
    int x, y;
    unsigned int req_width, req_height;
    int req_xoffset, req_yoffset;

{
    int total_offset = image->xoffset + req_xoffset;
    int left_pad = total_offset % dpy->bitmap_pad;
    int BytesPerRowOfImage;
    int BytesInRequestAvailableForImage;
    int MaximumRequestSizeInBytes =
        (65536 < dpy->max_request_size) ? (65536 << 2)
	 :( dpy->max_request_size << 2);

    if ((req_width == 0) || (req_height == 0)) return;
    
    BytesInRequestAvailableForImage = MaximumRequestSizeInBytes -
	sizeof(xPutImageReq);

    if (BytesInRequestAvailableForImage < (dpy->bitmap_pad >> 3))
    {
	/* This is a ridiculous case that should never happen, but just in
		case. */
		(void) fputs("Request size is simply too small to put image\n",
			stderr);
		exit(1);
    }
  
    BytesPerRowOfImage = BytesPerImageRow(dpy, image, req_width, req_xoffset);

    if ((BytesPerRowOfImage * req_height) <= BytesInRequestAvailableForImage)
    {
        PutImageRequest(dpy, d, gc, image, req_xoffset, req_yoffset, x, y, 
	    req_width, req_height);
    }
    else
    {
	if (req_height > 1)
        {
	    int SubImageHeight = BytesInRequestAvailableForImage /
		    BytesPerRowOfImage;

	    if (SubImageHeight < 1) SubImageHeight = 1;

	    PutSubImage(dpy, d, gc, image, req_xoffset, req_yoffset, x, y, 
		req_width, SubImageHeight);

	    PutSubImage(dpy, d, gc, image, req_xoffset, 
		req_yoffset + SubImageHeight, x, y + SubImageHeight, req_width,
		req_height - SubImageHeight);
	}
	else
	{
	    int SubImageWidth = ((BytesInRequestAvailableForImage << 3) /
		dpy->bitmap_pad) * dpy->bitmap_pad - left_pad;

	    PutSubImage(dpy, d, gc, image, req_xoffset, req_yoffset, x, y, 
		SubImageWidth, req_height);

	    PutSubImage(dpy, d, gc, image, req_xoffset + SubImageWidth, 
		req_yoffset, x + SubImageWidth, y, req_width - SubImageWidth, 
		req_height);
	}
    }
    return;
}

XPutImage (dpy, d, gc, image, req_xoffset,req_yoffset, x, y, req_width,
								req_height)
    register Display *dpy;
    Drawable d;
    GC gc;
    register XImage *image;
    int x, y;
    unsigned int req_width, req_height;
    int req_xoffset, req_yoffset;

{
    /* Does the display need to be locked and the GC flushed here? */
    LockDisplay(dpy);
    if ((req_width == 0) || (req_height == 0)) return;

    PutSubImage(dpy, d, gc, image, req_xoffset, req_yoffset, x, y, req_width,
	req_height);

    /* Does the display need to be unlocked here? Are any status values
	returned? */
    UnlockDisplay(dpy);
    return;
}
