#include "jerq.h"

main (argc, argv)
int argc;
char **argv;
{
   int r;

   request(KBD|MOUSE);
   initdisplay(argc, argv);
   for(;;){
      r = wait(MOUSE|KBD);
      if(button1()){
         rectf(&display, Drect, F_CLR);
         rectf(&display, Rect(Drect.origin.x, Drect.origin.y,
               mouse.xy.x, Drect.corner.y), F_XOR);
      } else if(button23())
         break;
      if(r&KBD && kbdchar() == 'y')
         string(&defont, "Hello", &display, Drect.origin, F_XOR);
   }
}
