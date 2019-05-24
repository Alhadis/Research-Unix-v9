#ifndef lint
static char rcsid[] = "$Header: NextEvent.c,v 1.8 87/09/12 16:12:50 swick Exp $";
#endif lint

/*
 * Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.
 * 
 *                         All Rights Reserved
 * 
 * Permission to use, copy, modify, and distribute this software and its 
 * documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in 
 * supporting documentation, and that the name of Digital Equipment
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.  
 * 
 * 
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */
#include <stdio.h>
#include <errno.h>
#include <X11/Xlib.h>
#include "Intrinsic.h"
#include <nlist.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/param.h>
#include "fd.h"

extern int errno;
extern void CallCallbacks(); /* gotten from Intrinsic.c, should be in .h ||| */
extern void RemoveAllCallbacks(); /* from Intrinsic.c, should be in .h ||| */

/*
 * Private definitions
 */

static struct timer_interval {
	struct timeval	ti_interval;
	struct timeval	ti_value;
};


	
static struct Timer_event {
  	struct timer_interval Te_tim;
	struct Timer_event *Te_next;
	Display *Te_dpy;
	Widget   Te_wID;
	XtIntervalId Te_id;
};

static struct Select_event {
	Widget	Se_wID;
	struct	Select_event	*Se_next;
	XClientMessageEvent	Se_event;
	struct  Select_event	*Se_oq;
};


/*
 * Private data
 */

static struct Timer_event *Timer_queue = NULL;
static struct Select_event *Select_rqueue[NOFILE], *Select_wqueue[NOFILE],
  *Select_equeue[NOFILE];
static struct timeval Timer_next = {0};
static struct  Select_event *outstanding_queue = NULL;
static struct 
{
  	Fd_set rmask;
	Fd_set wmask;
	Fd_set emask;
	int	nfds;
} composite;


  

/*
 * Private routines
 */

#define TIMEADD(dest, src1, src2) { \
	if(((dest)->tv_usec = (src1)->tv_usec + (src2)->tv_usec) >= 1000000) {\
	      (dest)->tv_usec -= 1000000;\
	      (dest)->tv_sec = (src1)->tv_sec + (src2)->tv_sec + 1 ; \
	} else { (dest)->tv_sec = (src1)->tv_sec + (src2)->tv_sec ; \
	   if(((dest)->tv_sec >= 1) && (((dest)->tv_usec <0))) { \
	    (dest)->tv_sec --;(dest)->tv_usec += 1000000; } } }


#define TIMEDELTA(dest, src1, src2) { \
	if(((dest)->tv_usec = (src1)->tv_usec - (src2)->tv_usec) < 0) {\
	      (dest)->tv_usec += 1000000;\
	      (dest)->tv_sec = (src1)->tv_sec - (src2)->tv_sec - 1;\
	} else 	(dest)->tv_sec = (src1)->tv_sec - (src2)->tv_sec;  }
#define ISAFTER(t1, t2) (((t2)->tv_sec > (t1)->tv_sec) ||(((t2)->tv_sec == (t1)->tv_sec)&& ((t2)->tv_usec > (t1)->tv_usec)))

static void ReQueueTimerEvent(ptr)
struct Timer_event *ptr;
{
	register struct Timer_event *tptr, *lptr;
	struct timeval cur_time, when, *inter;
	struct timezone curzone;
	
	(void) gettimeofday(&cur_time, &curzone);

	ptr->Te_tim.ti_value = ptr->Te_tim.ti_interval;
	inter = &(ptr->Te_tim.ti_value);
	TIMEADD(&when, &cur_time, inter);
	ptr->Te_tim.ti_value = when;
	if(Timer_queue == NULL)
	  {
		  TIMEADD(&Timer_next, inter, &cur_time)
		  Timer_next = when;
		  Timer_queue = ptr;
		  return;
	  }
	if (ISAFTER(&when, &Timer_next)) 
	  {
	  	/* This goes on the front */
		  ptr -> Te_next = Timer_queue;
		  Timer_next = when;
		  Timer_queue = ptr;
		  return;
	  }
	
	for(tptr = Timer_queue ; tptr ; tptr=tptr->Te_next )
	  {
		  if(ISAFTER( &(tptr->Te_tim.ti_value), &when)) {
			  lptr = tptr;
		  } else {
			  ptr->Te_next = tptr;
			  lptr->Te_next = ptr;
		  	  return;
		  }
	  }
	lptr->Te_next = ptr;
}     

/*
 * Public Routines
 */

XtIntervalId
XtAddTimeOut(wID, interval)
Widget wID;
int interval;
{
	struct Timer_event *tptr;
	
	tptr = (struct Timer_event *)XtMalloc(sizeof (*tptr));
	tptr->Te_next = NULL;
	tptr->Te_tim.ti_interval.tv_sec = interval/1000;
	tptr->Te_tim.ti_interval.tv_usec = (interval%1000)*1000;
	tptr->Te_wID = wID;
	tptr->Te_id = (XtIntervalId) tptr;

	ReQueueTimerEvent(tptr);
	return(tptr->Te_id);
}

/* this is obsolete */

unsigned long 
XtGetTimeOut(timer)
XtIntervalId timer;
{
	struct timeval sum, cur_time;
	struct timezone curzone;
	register struct Timer_event *tptr = (struct Timer_event *) timer;

	(void) gettimeofday(&cur_time, &curzone);
	
	if(tptr != (struct Timer_event *) tptr->Te_id) {
		XtError("Internal event timer botch.");
	}
	TIMEDELTA(&sum, &(tptr->Te_tim.ti_value), &cur_time);
	return((sum.tv_sec*1000)+(sum.tv_usec/1000));
}

void
XtRemoveTimeOut(iD)
XtIntervalId iD;
{
	register struct Timer_event *tptr,*lptr;
	lptr = NULL;
	
	for(tptr = Timer_queue ;tptr != NULL;tptr = tptr->Te_next) 
	  {	
		  if((struct Timer_event *) iD != tptr)
		    {
			    lptr = tptr;
			    continue;
		    }
		  else{
		    if(lptr)
		      lptr->Te_next = tptr->Te_next;

		    else 
		      Timer_queue = tptr->Te_next;

		    XtFree((char *) tptr);
		  }
	  }
}

void 
XtAddInput(widget,source,condition)
Widget widget;
int source;
int condition;
{
	struct Select_event *sptr;
	
	if(((int)condition &(XtInputReadMask|XtInputWriteMask|XtInputExceptMask))==0) {
	  return; /* error */ /* XXX */
	}
	if(condition&XtInputReadMask){
	    sptr = (struct Select_event *)XtMalloc(sizeof (*sptr));
	    sptr->Se_wID = widget;
	    sptr->Se_next = Select_rqueue[source];
	    Select_rqueue[source] = sptr;
	    FD_SET(source, &composite.rmask);
	    sptr->Se_event.type = ClientMessage;
	    sptr->Se_event.display = XtDisplay(widget);
	    sptr->Se_event.window = widget->core.window;
	    sptr->Se_event.message_type = XtHasInput;
	    sptr->Se_event.format = 32;
	    sptr->Se_event.data.l[0] = (int)source;
	    sptr->Se_event.data.l[1] = XtInputReadMask;
	}
	
	if(condition&XtInputWriteMask) {
	    sptr = (struct Select_event *) XtMalloc(sizeof (*sptr));
	    sptr->Se_wID = widget;
	    sptr->Se_next = Select_wqueue[source];
	    Select_wqueue[source] = sptr;
	    FD_SET(source, &composite.wmask);
	    sptr->Se_event.type = ClientMessage;
	    sptr->Se_event.display = XtDisplay(widget);
	    sptr->Se_event.window = widget->core.window;
	    sptr->Se_event.message_type = XtHasInput;
	    sptr->Se_event.format = 32;
	    sptr->Se_event.data.l[0] = (int)source;
	    sptr->Se_event.data.l[1] = XtInputWriteMask;
	}
	
	if(condition&XtInputExceptMask) {
	    sptr = (struct Select_event *) XtMalloc(sizeof (*sptr));
	    sptr->Se_wID = widget;
	    sptr->Se_next = Select_equeue[source];
	    Select_equeue[source] = sptr;
	    FD_SET(source, &composite.emask);
	    sptr->Se_event.type = ClientMessage;
	    sptr->Se_event.display = XtDisplay(widget);
	    sptr->Se_event.window = widget->core.window;
	    sptr->Se_event.message_type = XtHasInput;
	    sptr->Se_event.format = 32;
	    sptr->Se_event.data.l[0] = (int)source;
	    sptr->Se_event.data.l[1] = XtInputExceptMask;
	}
	if (composite.nfds < (source+1))
	    composite.nfds = source+1;
}

void XtRemoveInput(wID, source, condition)
Widget wID;
int source;
int condition;
{
  	register struct Select_event *sptr, *lptr;

	if(((int)condition &(XtInputReadMask|XtInputWriteMask|XtInputExceptMask))==0) {
	    return; /* error */ /* XXX */
	}
	if(condition&XtInputReadMask){
	    if((sptr = Select_rqueue[source]) == NULL)
	      return; /* error */ /* XXX */
	    for(lptr = NULL;sptr; sptr = sptr->Se_next){
		if(sptr->Se_wID == wID) {
		    if(lptr == NULL) {
			Select_rqueue[source] = sptr->Se_next;
			FD_CLR(source, &composite.rmask);
		    } else {
			lptr->Se_next = sptr->Se_next;
		    }
		    XtFree((char *) sptr);
		    return;
		}
		lptr = sptr;	      
	    }
	}
	if(condition&XtInputWriteMask){
	    if((sptr = Select_wqueue[source]) == NULL)
	      return; /* error */ /* XXX */
	    for(lptr = NULL;sptr; sptr = sptr->Se_next){
		if(sptr->Se_wID == wID) {
		    if(lptr == NULL){
			Select_wqueue[source] = sptr->Se_next;
			FD_CLR(source, &composite.wmask);
		    }else {
			lptr->Se_next = sptr->Se_next;
		    }
		    XtFree((char *) sptr);
		    return;
		}
		lptr = sptr;
	    }
	    
	}
	if(condition&XtInputExceptMask){
	    if((sptr = Select_equeue[source]) == NULL)
	      return; /* error */ /* XXX */
	    for(lptr = NULL;sptr; sptr = sptr->Se_next){
		if(sptr->Se_wID == wID) {
		    if(lptr == NULL){
			Select_equeue[source] = sptr->Se_next;
			FD_CLR(source, &composite.emask);
		    }else {
			lptr->Se_next = sptr->Se_next;
		    }
		    XtFree((char *) sptr);
		    return;
		}
		lptr = sptr;
	    }
	    
	}
	return;
	 /* error */
}


     
/*
 * XtNextEvent()
 * return next event;
 */

void XtNextEvent(event)
XEvent *event;
{
	struct Select_event *se_ptr;
	struct Timer_event *te_ptr;
	struct timeval cur_time;
	struct timezone cur_timezone;
	Fd_set rmaskfd, wmaskfd, emaskfd;
	int  nfound, i;
	struct timeval wait_time;
	struct timeval *wait_time_ptr;
	int Claims_X_is_pending = 0;
	XClientMessageEvent *ev = (XClientMessageEvent *)event;
	extern void perror(), exit();
	
    if (DestroyList != NULL) {
        CallCallbacks(&DestroyList, (Opaque)NULL);
        RemoveAllCallbacks(&DestroyList);
    }

    for(;;) {
        if(XPending(toplevelDisplay) || Claims_X_is_pending) {
	    XNextEvent(toplevelDisplay, event);
	    return;
	}
	if((se_ptr = outstanding_queue) != NULL) {
	    *event = *((XEvent *)&se_ptr->Se_event);
	    outstanding_queue = se_ptr->Se_oq;
	    return;
	}
	(void) gettimeofday(&cur_time, &cur_timezone);
	if(Timer_queue)  /* check timeout queue */
	  {
	      if (ISAFTER(&Timer_next, &cur_time)){
		  /* timer has expired */
		  ev->type = ClientMessage;
		  ev->display = toplevelDisplay;
		  ev->window =  Timer_queue->Te_wID->core.window;
		  ev->message_type = XtTimerExpired;
		  ev->format = 32;
		  ev->data.l[0] = (int)Timer_queue->Te_id;
		  te_ptr = Timer_queue;
		  Timer_queue = Timer_queue->Te_next;
		  te_ptr->Te_next = NULL;
		  if(Timer_queue) /* set up next time out time */
		    Timer_next = Timer_queue->Te_tim.ti_value;
		  ReQueueTimerEvent(te_ptr);
		  return;
	      }
      }/* No timers ready time to wait */
		/* should be done only once */
	if(ConnectionNumber(toplevelDisplay) +1 > composite.nfds) 
	  composite.nfds = ConnectionNumber(toplevelDisplay) + 1;
	while(1) {
		FD_SET(ConnectionNumber(toplevelDisplay),&composite.rmask);
		if (Timer_queue) {
			TIMEDELTA(&wait_time, &Timer_next, &cur_time);
			wait_time_ptr = &wait_time;
		} else 
		  wait_time_ptr = (struct timeval *)0;
		rmaskfd = composite.rmask;
		wmaskfd = composite.wmask;
		emaskfd = composite.emask;
		if((nfound=select(composite.nfds,
				  (int *)&rmaskfd,(int *)&wmaskfd,
				  (int *)&emaskfd,wait_time_ptr)) == -1) {
			if(errno == EINTR)
			  continue;
		}
		if(nfound == -1) {
			perror("select:");
			exit(-1);
		}
		break;

	}
	if(nfound == 0)
	  continue;
	for(i = 0; i < composite.nfds && nfound > 0;i++) {
	    if(FD_ISSET(i,&rmaskfd)) {
	      if(i == ConnectionNumber(toplevelDisplay)){
		Claims_X_is_pending= 1;
		nfound--;
	      } else {
		Select_rqueue[i] -> Se_oq = outstanding_queue;
		outstanding_queue = Select_rqueue[i];
		nfound--;
	      }
	    }
	    if(FD_ISSET(i,&wmaskfd)) {
	      Select_rqueue[i] -> Se_oq = outstanding_queue;
	      outstanding_queue = Select_rqueue[i];
	      nfound--;
	    }
	    if(FD_ISSET(i,&emaskfd)) {
	      Select_rqueue[i] -> Se_oq = outstanding_queue;
	      outstanding_queue = Select_rqueue[i];
	      nfound--;
	    }
	 }
    }
}


Boolean XtPending()
{
    Fd_set rmask, wmask, emask;
    struct timeval cur_time, wait_time;
    struct timezone curzone;
    Boolean ret;

    (void) gettimeofday(&cur_time, &curzone);
    
    if(ret = (Boolean) XPending(toplevelDisplay))
      return(ret);

    if(outstanding_queue)
      return TRUE;
    
    if(ISAFTER(&cur_time, &(Timer_queue->Te_tim.ti_value)))
	return TRUE;

    FD_SET(ConnectionNumber(toplevelDisplay),&composite.rmask); /*should be done only once */
    if(ConnectionNumber(toplevelDisplay) +1 > composite.nfds) 
      composite.nfds = ConnectionNumber(toplevelDisplay) + 1;
    wait_time.tv_sec = 0;
    wait_time.tv_usec = 0;
    rmask = composite.rmask;
    wmask = composite.wmask;
    emask = composite.emask;
    if(select(composite.nfds,(int *)&rmask,(int *)&wmask,(int*)&emask,&wait_time) > 0)
	return TRUE;
      
    return FALSE;  
}	

XtPeekEvent(event)
XEvent *event;
{
    Fd_set rmask, wmask, emask;
    int nfound, i;
    struct timeval cur_time, wait_time;
    struct timezone curzone;
    int Claims_X_is_pending = 0;
    XClientMessageEvent *ev = (XClientMessageEvent *)event;

    if(XPending(toplevelDisplay)){
	XPeekEvent(toplevelDisplay, event); /* Xevents */
	return(1);
    }
    if(outstanding_queue){
	*event = *((XEvent *)&outstanding_queue->Se_event);
	return(1);
    }
    (void) gettimeofday(&cur_time, &curzone);
    if(ISAFTER(&cur_time, &(Timer_queue->Te_tim.ti_value))) {
	ev->type = ClientMessage;
	ev->display = toplevelDisplay;
	ev->window =  Timer_queue->Te_wID->core.window;
		  ev->format = 32;
	ev->message_type = XtTimerExpired;
	ev->format = 32;
	ev->data.l[0] = (int)Timer_queue->Te_id;
	return(1);
    }
    
    FD_SET(ConnectionNumber(toplevelDisplay),&composite.rmask);/* should be done only once */
    if(ConnectionNumber(toplevelDisplay) +1 > composite.nfds) 
      composite.nfds = ConnectionNumber(toplevelDisplay) + 1;
    TIMEDELTA(&wait_time, &Timer_next, &cur_time);
    rmask = composite.rmask;
    wmask = composite.wmask;
    emask = composite.emask;
    nfound=select(composite.nfds,(int *)&rmask,(int *)&wmask,(int *)&emask,&wait_time);
    
    for(i = 0; i < composite.nfds && nfound > 0;i++) {
	if(FD_ISSET(i,&rmask)) {
	    if(i == ConnectionNumber(toplevelDisplay)) {
		Claims_X_is_pending= 1;
	      } else {
		Select_rqueue[i] -> Se_oq = outstanding_queue;
		outstanding_queue = Select_rqueue[i];
		nfound--;
	      }
	}
	if(FD_ISSET(i,&wmask)) {
	    Select_rqueue[i] -> Se_oq = outstanding_queue;
	    outstanding_queue = Select_rqueue[i];
	    nfound--;
	}
	if(FD_ISSET(i,&emask)) {
	    Select_rqueue[i] -> Se_oq = outstanding_queue;
	    outstanding_queue = Select_rqueue[i];
	    nfound--;
	}

      }
    if(Claims_X_is_pending && XPending(toplevelDisplay)) {
      XPeekEvent(toplevelDisplay, event);
      return(1);
    }

    if(outstanding_queue){
	*event = *((XEvent *)&outstanding_queue->Se_event);
	return(1);
    }
    return(0);
}	
	  
	
