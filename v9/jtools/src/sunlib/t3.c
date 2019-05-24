/*
	sliderbar. tests mouse input and rectf
*/
#include "jerq.h"

main (argc, argv)
char **argv;
{
	static oldx;

	request(KBD|MOUSE);
	initdisplay(argc, argv);
	for(;;){
		jnap(1);
		if(button1()){
			if(oldx == mouse.xy.x)
				continue;
			if(oldx < mouse.xy.x){
				rectf(&display, Rect(oldx, Drect.origin.y,
				mouse.xy.x, Drect.corner.y), F_XOR);
			}
			else{
				rectf(&display, Rect(mouse.xy.x,
				Drect.origin.y, oldx, Drect.corner.y),
				F_XOR);
			}
			oldx = mouse.xy.x;
		}
		else if(button23()) {
			break;
		}
	}
}

