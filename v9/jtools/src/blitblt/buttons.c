#include <jerq.h>

#define	UP	0
#define	DOWN	1

lbuttons(updown)
int updown;
{
	Rectangle r, s, kbdrect();

	while((button123()!=0) != updown){
		checkshape(0);
		wait(MOUSE);
	}

	switch (button123()) {
		case 4:
			return 1;
		case 2:
			return 2;
		case 1:
			return 3;
	}
	return 0;
}

lexit3()	/* return true if button3 is clicked */
{
	extern Cursor skull; Cursor *prev; int lexit;
	prev=cursswitch(&skull);
	lexit=lbuttons(DOWN); lbuttons(UP);
	cursswitch(prev);
	return(lexit == 3);
}
