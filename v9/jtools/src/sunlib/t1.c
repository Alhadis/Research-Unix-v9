/*
	test circle graphics routines: circle ellipse, arc, elarc, disc,
		eldisc, and also string, segment, and cursswitch
*/
#include "jerq.h"
short coffeecup_bits[]={
	0x0100, 0x00E0, 0x0010, 0x03E0, 0x0400, 0x0FE0, 0x123C, 0x1FE2,
	0x101A, 0x101A, 0x1002, 0x103C, 0x1810, 0x6FEC, 0x4004, 0x3FF8,
};

main(argc, argv)
char *argv[];
{
	int x,y;
	Point p;
	Cursor *prev;
	Cursor cup;

	request(MOUSE);
	initdisplay(argc, argv);
	cup = ToCursor(coffeecup_bits, coffeecup_bits, 7, 7);
 	x = Drect.origin.x + Drect.corner.x / 2;
	y = Drect.origin.y + Drect.corner.y / 2;
	eldisc(&display, Pt(170,170), 50, 25, F_XOR);
	disc(&display, Pt(60,170), 50, F_XOR);
	circle(&display, Pt(60,60), 50, F_XOR);
	ellipse(&display, Pt(170,60), 50, 25, F_XOR);
	eldisc(&display, Pt(170,170), 50, 25, F_XOR);
	disc(&display, Pt(60,170), 50, F_XOR);
	disc(&display, Pt(60,60), 50, F_XOR);
	eldisc(&display, Pt(170,60), 50, 25, F_XOR);
	circle(&display, Pt(x,y), 10, F_XOR);
	circle(&display, Pt(x,y), 20, F_STORE);
	circle(&display, Pt(x,y), 30, F_XOR);
	circle(&display, Pt(x,y), 40, F_OR);
	circle(&display, Pt(x,y), 50, F_XOR);
	ellipse(&display, Pt(x,y), 300, 100, F_STORE);
	ellipse(&display, Pt(x,y), 100, 300, F_XOR);
	elarc(&display, Pt(x,y), 200, 100,Pt(x+200,y), Pt(x,y+200), F_STORE);
	elarc(&display, Pt(x,y), 100,200, Pt(x-250,y), Pt(x,y-250), F_XOR);
	arc(&display, Pt(x,y), Pt(x+200,y), Pt(x,y+200),F_XOR);
	arc(&display, Pt(x,y), Pt(x-250,y), Pt(x,y-250),F_XOR);
	disc(&display, Pt(x,y), 100, F_XOR);
	disc(&display, Pt(x,y), 200, F_XOR);
	disc(&display, Pt(x,y), 300, F_XOR);
	disc(&display, Pt(x,y), 400, F_XOR);
	eldisc(&display, Pt(x,y), 300, 100, F_XOR);
	eldisc(&display, Pt(x,y), 100, 300, F_XOR);
	segment(&display, Pt(x-100,y-100),Pt(x+200,y+200), F_XOR);
	p = string(&defont, "hello world", &display, Pt(x,y), F_XOR);
	p = string(&defont, "Goodbye world", &display, p, F_XOR);
	for(;;jnap(1)){
		if(button1())
			prev = cursswitch(&cup);
		if(button2())
			prev = cursswitch(prev);
		if(button3())
			break;
	}
}
