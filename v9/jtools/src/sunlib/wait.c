#include "jerq.h"
#include "rcv.h"

unsigned alarmtime, alarmstart;

wait (resource)
{
	int maxfd, smask, ret;
	unsigned diff;
#ifdef	BSD
	struct timeval tv;
#endif	BSD

	maxfd = displayfd + 1;
	for(;;){
#ifdef SUNTOOLS
		if (damagedone) {
			fixdamage();
			break;
		}
#endif SUNTOOLS
		if (alarmtime) {
			diff = realtime() - alarmstart;
			if (diff >= alarmtime) {
				alarmtime = 0;
				P->state |= ALARM;
			} else {
				alarmstart += diff;
				alarmtime -= diff;
			}
		}
		if(P->state & resource)
			break;
#ifdef X11
		if(XPending(dpy))
			goto xin;
#endif X11
		smask = (1 << displayfd);
		if (!Jrcvbuf.blocked)
			smask |= jerqrcvmask;
		if (resource & CPU) {
#ifdef	BSD
			tv.tv_sec = 0;
			tv.tv_usec = 0;
			ret = select(maxfd, &smask, 0, 0, &tv);
#else	/* V9 */
			ret = select(maxfd, &smask, 0, 0);
#endif
			if (ret == 0)
				break;
		} else if (alarmtime) {
#ifdef	BSD
			tv.tv_sec = alarmtime/60;
			tv.tv_usec = (alarmtime%60) * 16666;
			ret = select(maxfd, &smask, 0, 0, &tv);
#else	/* V9 */
			ret = select(maxfd, &smask, 0, alarmtime*17);
#endif
			if (ret == 0) {
				alarmtime = 0;
				P->state |= ALARM;
				continue;
			}
		} else {
#ifdef	BSD
			ret = select(maxfd, &smask, 0, 0, 0);
#else	/* V9 */
			ret = select(maxfd, &smask, 0, 0x6fffffff);
#endif
		}
		if (ret == -1)
			continue;
		if(smask & jerqrcvmask)
			rcvfill();
		if(smask & (1 << displayfd)){
xin:
			handleinput();
			if(resource & MOUSE) /* We always have the mouse */
				break;
		}
	}
	return resource & (P->state|MOUSE);
}

nap (n)
int n;
{
	wait(MOUSE);
}

alarm(ticks)
{
	alarmtime = ticks;
	alarmstart = realtime();
	P->state &= ~ALARM;
}

sleep(ticks)
{
#ifdef BSD
	int maxfd, smask, ret;
	unsigned diff = 0, tleft;
	struct timeval tv;
	unsigned start = realtime();

	maxfd = displayfd + 1;
	for(tleft = ticks; diff < tleft; ) {
		tleft -= diff;
#ifdef X11
		if(XPending(dpy))
			goto xin;
#endif X11
damage:
#ifdef SUNTOOLS
		if (damagedone) {
			fixdamage();
			return;
		}
#endif SUNTOOLS
		smask = (1 << displayfd);
		if (!Jrcvbuf.blocked)
			smask |= jerqrcvmask;
		tv.tv_sec = tleft / 60;
		tv.tv_usec = (tleft % 60) * 16666;
		ret = select(maxfd, &smask, 0, 0, &tv);
		if (ret == 0)
			break;
		if (ret == -1)
			goto damage;
		if(smask & jerqrcvmask)
			rcvfill();
		if(smask & (1 << displayfd)){
xin:
			handleinput();
		}
		diff = realtime() - start;
		start += diff;
	}
	if (alarmtime) {
		if (ticks >= alarmtime) {
			alarmtime = 0;
			P->state |= ALARM;
		} else {
			alarmstart += ticks;
			alarmtime -= ticks;
		}
	}
#else
	wait(CPU);
#undef nap
	nap(ticks);
	wait(CPU);
	if (alarmtime) {
		int diff = realtime() - alarmstart;
		if (diff >= alarmtime) {
				alarmtime = 0;
				P->state |= ALARM;
		}
		alarmstart += diff;
		alarmtime -= diff;
	}
#endif BSD
}

#ifndef BSD
#include <sys/timeb.h>
#endif
realtime()
{
#ifdef BSD
	struct timeval tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec * 60 + (tv.tv_usec * 60 / 1000000);
#else
	struct timeb tb;
	ftime(&tb);
	return tb.time * 60 + (tb.millitm * 60 / 1000);
#endif BSD
}
