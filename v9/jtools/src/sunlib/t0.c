/*
	t0:	bitblt graphics tests
*/
#include "jerq.h"
short coffeecup_bits[]={
	0x0100, 0x00E0, 0x0010, 0x03E0, 0x0400, 0x0FE0, 0x123C, 0x1FE2,
	0x101A, 0x101A, 0x1002, 0x103C, 0x1810, 0x6FEC, 0x4004, 0x3FF8,
};

main(argc, argv)
char **argv;
{
	Rectangle r;
	Bitmap *b64, *bmap;
	Texture cup;

	request(KBD|MOUSE);
	initdisplay(argc, argv);
			/* allocate and tile bitmap	*/
	b64 = balloc(Rect(0,0,64,64));
	bmap = balloc(Rect(0,0,64,64));
	cup = ToTexture(coffeecup_bits);
	texture(b64, Rect(0,0,64,64), &cup, F_STORE);
			/* add some stripes		*/
	rectf(b64, Rect(0,0,32,64), F_XOR);
	rectf(&display, Rect(0,0,256,32), F_STORE);
	rectf(&display, Rect(0,64,256,96), F_STORE);
			/* bitblt pr to pw		*/
	bitblt(b64, b64->rect, &display, Pt(0,0), F_STORE);
	bitblt(b64, b64->rect, &display, Pt(0,64), F_XOR);
	bitblt(b64, b64->rect, &display, Pt(64,0), F_CLR);
	bitblt(b64, b64->rect, &display, Pt(64,64), F_OR);
			/* bitblt pw to pw		*/
	bitblt(&display, Rect(0,0,64,64), &display, Pt(128,0), F_STORE);
	bitblt(&display, Rect(0,0,64,64), &display, Pt(128,64), F_XOR);
	bitblt(&display, Rect(0,0,64,64), &display, Pt(192,0), F_CLR);
	bitblt(&display, Rect(0,0,64,64), &display, Pt(192,64), F_OR);
			/* bitblt pr to pr		*/
	bitblt(b64, b64->rect, bmap, Pt(0,0), F_STORE);
	bitblt(bmap, bmap->rect, &display, Pt(0,128), F_STORE);
	rectf(bmap, bmap->rect, F_CLR);
	bitblt(b64, b64->rect, bmap, Pt(0,0), F_XOR);
	bitblt(bmap, bmap->rect, &display, Pt(0,192), F_STORE);
	rectf(bmap, bmap->rect, F_CLR);
	bitblt(b64, b64->rect, bmap, Pt(0,0), F_CLR);
	bitblt(bmap, bmap->rect, &display, Pt(64,128), F_STORE);
	rectf(bmap, bmap->rect, F_CLR);
	bitblt(b64, b64->rect, bmap, Pt(0,0), F_OR);
	bitblt(bmap, bmap->rect, &display, Pt(64,192), F_STORE);
			/* bitblt pw to pr		*/
	bitblt(&display, Rect(0,0,64,64), bmap, Pt(0,0), F_STORE);
	bitblt(bmap, bmap->rect, &display, Pt(128,128), F_STORE);
	rectf(bmap, bmap->rect, F_CLR);
	bitblt(&display, Rect(0,0,64,64), bmap, Pt(0,0), F_XOR);
	bitblt(bmap, bmap->rect, &display, Pt(128,192), F_STORE);
	rectf(bmap, bmap->rect, F_CLR);
	bitblt(&display, Rect(0,0,64,64), bmap, Pt(0,0), F_CLR);
	bitblt(bmap, bmap->rect, &display, Pt(192,128), F_STORE);
	rectf(bmap, bmap->rect, F_CLR);
	bitblt(&display, Rect(0,0,64,64), bmap, Pt(0,0), F_OR);
	bitblt(bmap, bmap->rect, &display, Pt(192,192), F_STORE);

			/* points	*/
	point(&display, Pt(256,256), F_STORE);
	point(&display, Pt(257,256), F_XOR);
	point(&display, Pt(258,256), F_OR);
	point(&display, Pt(259,256), F_CLR);
	rectf(bmap, bmap->rect, F_CLR);
	point(bmap, Pt(0,0), F_STORE);
	point(bmap, Pt(1,0), F_XOR);
	point(bmap, Pt(2,0), F_OR);
	point(bmap, Pt(3,0), F_CLR);
	bitblt(bmap, bmap->rect, &display, Pt(260,256), F_STORE);

			/* rectf	*/
	rectf(&display, Rect(0,256,32,288), F_STORE);
	rectf(&display, Rect(32,256,64,288), F_XOR);
	rectf(&display, Rect(0,288,32,320), F_OR);
	rectf(&display, Rect(32,288,64,320), F_CLR);
	rectf(bmap, bmap->rect, F_CLR);
	rectf(bmap, Rect(0,0,32,32), F_STORE);
	rectf(bmap, Rect(32,0,64,32), F_XOR);
	rectf(bmap, Rect(0,32,32,64), F_OR);
	rectf(bmap, Rect(32,32,64,64), F_CLR);
	bitblt(bmap, bmap->rect, &display, Pt(64,256), F_STORE);
			/* screenswap	*/
	screenswap(&display, Rect(0,0,128,128), Rect(0,256,128,384));
			/* texture	*/
	texture(&display, Rect(0,0,64,64), &cup, F_STORE);
	texture(&display, Rect(0,64,64,128), &cup, F_XOR);
	texture(&display, Rect(64,0,128,64), &cup, F_OR);
	texture(&display, Rect(64,64,128,128), &cup, F_CLR);
	for(;;jnap(1)){
		if(button123())
			break;
	}
}
