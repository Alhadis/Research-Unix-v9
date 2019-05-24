/*
	test for getrect, ngetrect, texture, mouse input, rectf
*/
#include "jerq.h"
short darkgrey_bits[] ={
	0xDDDD, 0x7777, 0xDDDD, 0x7777, 0xDDDD, 0x7777, 0xDDDD, 0x7777,
	0xDDDD, 0x7777, 0xDDDD, 0x7777, 0xDDDD, 0x7777, 0xDDDD, 0x7777,
};

main (argc, argv)
char **argv;
{
	Texture darkgrey;
	Rectangle r;

	request(KBD|MOUSE|ALARM);
	initdisplay(argc, argv);
	darkgrey = ToTexture(darkgrey_bits);
	alarm(60);
	for( ; ; wait(MOUSE|ALARM)) {
		if(button1()){
			ngetrect(&r, 0, 1, 0, 10, 10);
			rectf(&display, r, F_XOR);
		}
		else if(button2()){
			ngetrect(&r, 0, 2, 0, 10, 10);
			texture(&display, r, &darkgrey, F_XOR);
		}
		else if(button3())
			break;
		if (own() & ALARM) {
			rectf(&display, Rect(0, 0, 100, 100), F_XOR);
			rectf(&display,
			Rect(Drect.cor.x-100, Drect.cor.y-100, Drect.cor.x, Drect.cor.y), F_XOR);
			alarm(60);
		}
			
	}
}
