/*
	test for getrect, ngetrect, texture, mouse input, rectf
*/
#include "jerq.h"
static short darkgrey_bits[] ={
	0xDDDD, 0x7777, 0xDDDD, 0x7777, 0xDDDD, 0x7777, 0xDDDD, 0x7777,
	0xDDDD, 0x7777, 0xDDDD, 0x7777, 0xDDDD, 0x7777, 0xDDDD, 0x7777,
};

main (argc, argv)
char **argv;
{
	static Texture darkgrey;
	Rectangle r;

	request(MOUSE);
	initdisplay(argc, argv);
	darkgrey = ToTexture(darkgrey_bits);
	/* ngetrect allows optional blocking so this can work */
	for(;;jnap(1)){
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
	}
}

