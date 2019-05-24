#include "jerq.h"

static char *editstrs[]={
	"cut",
	"paste",
	"snarf",
	"send",
	"cut",
	"paste",
	"snarf",
	"send",
	"cut",
	"paste",
	"snarf",
	"send",
	"paste",
	"snarf",
	"send",
	"cut",
	"paste",
	"snarf",
	"send",
	0,
};
static	Menu	editmenu = {editstrs};

main(argc, argv)
char **argv;
{
	int i, h, w, y;
	static char *s = "Hello world";
	char c[2];

	request(KBD|MOUSE);
	initdisplay(argc, argv);

	c[1] = 0;
	h = fontheight(&defont);
	y = h + 5;
	w = strwidth(&defont, "m");
	for(i = 0; ; ){
		wait(MOUSE|KBD);
		if(P->state & KBD){
			c[0] = kbdchar();
			string(&defont, c, &display, Pt(10+w*i, y), F_OR);
			i++;
			if(i > 50){
				i = 0;
				y += h;
			}
		}
		if(button2())
			menuhit(&editmenu, 2);
		else if(button3())
			break;
	}
}
