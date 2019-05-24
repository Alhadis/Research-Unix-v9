#include "process.pub"
#include "frame.pri"
#include "symtab.pri"
#include "symbol.h"
#include "hostcore.h"
#include "asm.pri"
#include "format.pub"
#include "bpts.pri"
#include "master.pri"
SRCFILE("waitlist.c")

const int STOPPED=0, RUNNING=1, EXITED=2, DIED=3;

void WaitList::add(HostCore* h)
{
	WaitMem	*wm = new WaitMem;
	wm->core = h;
	wm->next = head;
	wm->changed = 0;
	head = wm;
}

void WaitList::remove(HostCore* h)
{
	WaitMem	*p = 0, *n = head;
	while (n && (n->core != h)) {
		p = n;
		n = p->next;
	}
	if (!n)
		return;
	if (!p)
		head = n->next;
	else
		p->next = n->next;
	delete n;
}

int wait3(_wait*, int, int);
int wait(_wait*);

int WaitList::wait(HostCore* h, int flags)
{
	WaitMem	*p = head;
	while (p && (p->core != h))
		p = p->next;
	if (!p)
		return 0;
	// It may have already occured 
	if (p->changed) {
		p->changed = 0;
out:
		if (flags & WAIT_DISCARD)
			return 1;
		h->state = STOPPED;
		if (WIFSTOPPED(p->status)) {
			h->cursig = p->status.w_stopsig;
			if (h->cursig == SIGTRAP && (flags & WAIT_PCFIX))
				h->regpoke(h->REG_PC(), h->pc() - 2);
		} else {
			if (WIFSIGNALED(p->status)) {
				h->state = DIED;
				h->cursig = p->status.w_termsig;
			} else {
				h->state = EXITED;
				h->cursig = p->status.w_retcode;
			}
		}
		return 1;
	}
	// Must really wait for it
	for(;;) {
		int pid;
		_wait tstat;
		if (flags & WAIT_POLL) {
			if ((pid = ::wait3(&tstat, WNOHANG, 0)) <= 0)
				return 0;
		} else  {
			if ((pid = ::wait(&tstat)) == -1)
				return 0;
		}
		WaitMem	*q = head;
		for ( ; q; q = q->next)
			if (q->core->pid == pid) {
				q->status = tstat;
				if (q == p)
					goto out;
				q->changed = 1;
				break;
			}
	}
}
