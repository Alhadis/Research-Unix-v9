/* $Header: NextEvent.c,v 1.4 87/08/28 16:52:08 toddb Exp $ */
#ifndef lint
static char *sccsid = "@(#)NextEvent.c	1.15	2/25/87";
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
#include "Xlib.h"
#include "Intrinsic.h"
#include <sys/time.h>
#include <sys/file.h>
#include <sys/param.h>
#include "fd.h"
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
	Window	Te_wID;
	caddr_t	Te_cookie;
};

static struct Select_event {
	Window	Se_wID;
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

void XtSetTimeOut(wID, cookie, interval)
Window wID;
caddr_t	cookie;
int interval;
{
	struct Timer_event *tptr;
	
	tptr = (struct Timer_event *)XtMalloc(sizeof (*tptr));
	tptr->Te_next = NULL;
	tptr->Te_tim.ti_interval.tv_sec = interval/1000;
	tptr->Te_tim.ti_interval.tv_usec = (interval%1000)*1000;
	tptr->Te_wID = wID;
	tptr->Te_cookie = cookie;
	ReQueueTimerEvent(tptr);
}

int XtGetTimeOut(wID, cookie)
Window wID;
caddr_t cookie;
{
	struct timeval sum, cur_time;
	struct timezone curzone;
	register struct Timer_event *tptr;

	(void) gettimeofday(&cur_time, &curzone);
	
	for(tptr = Timer_queue; tptr; tptr = tptr->Te_next) 
	  {	
		  if(wID == tptr->Te_wID && cookie == tptr->Te_cookie){
		    TIMEDELTA(&sum, &(tptr->Te_tim.ti_value), &cur_time);
		    return((sum.tv_sec*1000)+(sum.tv_usec/1000));
		  }
		  
	  }
	return(-1);
}

int XtClearTimeOut(wID, cookie)
Window wID;
caddr_t cookie;
{
	register struct Timer_event *tptr,*lptr;
	lptr = NULL;
	
	for(tptr = Timer_queue ;tptr != NULL;tptr = tptr->Te_next) 
	  {	
		  if(wID != tptr->Te_wID || cookie != tptr->Te_cookie)
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
		    return(0);
		  }
	  }
	return(-1);
}

void XtAddInput(dpy, wID, source, condition)
Display *dpy;
Window wID;
int source;
int condition;
{
	struct Select_event *sptr;
	
	if(((int)condition &(XtINPUT_READ|XtINPUT_WRITE|XtINPUT_EXCEPT))==0) {
	  return; /* error */ /* XXX */
	}
	if(condition&XtINPUT_READ){
	    sptr = (struct Select_event *)XtMalloc(sizeof (*sptr));
	    sptr->Se_wID = wID;
	    sptr->Se_next = Select_rqueue[source];
	    Select_rqueue[source] = sptr;
	    FD_SET(source, &composite.rmask);
	    sptr->Se_event.type = ClientMessage;
	    sptr->Se_event.display = dpy;
	    sptr->Se_event.window = wID;
	    sptr->Se_event.message_type = XtHasInput;
	    sptr->Se_event.format = 32;
	    sptr->Se_event.data.l[0] = (int)source;
	    sptr->Se_event.data.l[1] = XtINPUT_READ;
	}
	
	if(condition&XtINPUT_WRITE) {
	    sptr = (struct Select_event *) XtMalloc(sizeof (*sptr));
	    sptr->Se_wID = wID;
	    sptr->Se_next = Select_wqueue[source];
	    Select_wqueue[source] = sptr;
	    FD_SET(source, &composite.wmask);
	    sptr->Se_event.type = ClientMessage;
	    sptr->Se_event.display = dpy;
	    sptr->Se_event.window = wID;
	    sptr->Se_event.message_type = XtHasInput;
	    sptr->Se_event.format = 32;
	    sptr->Se_event.data.l[0] = (int)source;
	    sptr->Se_event.data.l[1] = XtINPUT_WRITE;
	}
	
	if(condition&XtINPUT_EXCEPT) {
	    sptr = (struct Select_event *) XtMalloc(sizeof (*sptr));
	    sptr->Se_wID = wID;
	    sptr->Se_next = Select_equeue[source];
	    Select_equeue[source] = sptr;
	    FD_SET(source, &composite.emask);
	    sptr->Se_event.type = ClientMessage;
	    sptr->Se_event.display = dpy;
	    sptr->Se_event.window = wID;
	    sptr->Se_event.message_type = XtHasInput;
	    sptr->Se_event.format = 32;
	    sptr->Se_event.data.l[0] = (int)source;
	    sptr->Se_event.data.l[1] = XtINPUT_EXCEPT;
	}
	if (composite.nfds < (source+1))
	    composite.nfds = source+1;
}

void XtRemoveInput(wID, source, condition)
Window wID;
int source;
int condition;
{
  	register struct Select_event *sptr, *lptr;

	if(((int)condition &(XtINPUT_READ|XtINPUT_WRITE|XtINPUT_EXCEPT))==0) {
	    return; /* error */ /* XXX */
	}
	if(condition&XtINPUT_READ){
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
	if(condition&XtINPUT_WRITE){
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
	if(condition&XtINPUT_EXCEPT){
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

void XtNextEvent(dpy, event)
Display *dpy;
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
	extern int errno;
	
    for(;;) {
        if(XPending(dpy) || Claims_X_is_pending) {
	    XNextEvent(dpy, event);
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
		  ev->display = dpy;
		  ev->window =  Timer_queue->Te_wID;
		  ev->message_type = XtTimerExpired;
		  ev->format = 32;
		  ev->data.l[0] = (int)Timer_queue->Te_cookie;
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
	if(ConnectionNumber(dpy) +1 > composite.nfds) 
	  composite.nfds = ConnectionNumber(dpy) + 1;
	while(1) {
		FD_SET(ConnectionNumber(dpy),&composite.rmask);
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
	      if(i == ConnectionNumber(dpy)){
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


XtPending(dpy)
Display *dpy;
{
    Fd_set rmask, wmask, emask;
    struct timeval cur_time, wait_time;
    struct timezone curzone;
    int ret;
    Fd_set *compositeRmask = &composite.rmask;

    (void) gettimeofday(&cur_time, &curzone);
    
    if(ret = XPending(dpy))
      return(ret);

    if(outstanding_queue)
      return(1);
    
    if(ISAFTER(&cur_time, &(Timer_queue->Te_tim.ti_value)))
	return(1);

    FD_SET(ConnectionNumber(dpy),compositeRmask); /*should be done only once */
    if(ConnectionNumber(dpy) +1 > composite.nfds) 
      composite.nfds = ConnectionNumber(dpy) + 1;
    wait_time.tv_sec = 0;
    wait_time.tv_usec = 0;
    rmask = composite.rmask;
    wmask = composite.wmask;
    emask = composite.emask;
    if(select(composite.nfds,(int *)&rmask,(int *)&wmask,(int*)&emask,&wait_time) > 0)
	return(1);
      
    return(0);  
}	

XtPeekEvent(dpy, event)
Display *dpy;
XEvent *event;
{
    Fd_set rmask, wmask, emask;
    int nfound, i;
    struct timeval cur_time, wait_time;
    struct timezone curzone;
    int Claims_X_is_pending = 0;
    XClientMessageEvent *ev = (XClientMessageEvent *)event;
    Fd_set *compositeRmask = &composite.rmask;

    if(XPending(dpy)){
	XPeekEvent(dpy, event); /* Xevents */
	return(1);
    }
    if(outstanding_queue){
	*event = *((XEvent *)&outstanding_queue->Se_event);
	return(1);
    }
    (void) gettimeofday(&cur_time, &curzone);
    if(ISAFTER(&cur_time, &(Timer_queue->Te_tim.ti_value))) {
	ev->type = ClientMessage;
	ev->display = dpy;
	ev->window =  Timer_queue->Te_wID;
		  ev->format = 32;
	ev->message_type = XtTimerExpired;
	ev->format = 32;
	ev->data.l[0] = (int)Timer_queue->Te_cookie;
	return(1);
    }
    
    FD_SET(ConnectionNumber(dpy),compositeRmask);/* should be done only once */
    if(ConnectionNumber(dpy) +1 > composite.nfds) 
      composite.nfds = ConnectionNumber(dpy) + 1;
    TIMEDELTA(&wait_time, &Timer_next, &cur_time);
    rmask = composite.rmask;
    wmask = composite.wmask;
    emask = composite.emask;
    nfound=select(composite.nfds,(int *)&rmask,(int *)&wmask,(int *)&emask,&wait_time);
    
    for(i = 0; i < composite.nfds && nfound > 0;i++) {
	if(FD_ISSET(i,&rmask)) {
	    if(i == ConnectionNumber(dpy)) {
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
    if(Claims_X_is_pending && XPending(dpy)) {
      XPeekEvent(dpy, event);
      return(1);
    }

    if(outstanding_queue){
	*event = *((XEvent *)&outstanding_queue->Se_event);
	return(1);
    }
    return(0);
}	
	  
	
