/*
    malloc.h - definitions for memory allocation

	author: WJHansen, CMU/ITC
	(c) Copyright IBM Corporation, 1986
*/

/*
 *	a different implementation may need to redefine
 *	INT WORD BLOCK ACTIVE PREACTIVE
 *	where INT is integer type to which a pointer can be cast
 *	and INT is the number of bytes given by WORD
 *	WORD needs to be at least 4 so there are two zero bits
 *	at the bottom of a Size field
 */

#define INT int
#define WORD 4
#define EPSILON  (sizeof(struct freehdr)+sizeof(struct freetrlr))
#define SEGGRAIN 4096 /* granularity for sbrk requests (in bytes) */
#define ACTIVE    0x1
#define PREACTIVE 0x2
#define testbit(p, b) ((p)&(b))
#define setbits(p, b) ((p)|(b))
#define clearbits(p) ((p)&(~ACTIVE)&(~PREACTIVE))
#define clearbit(p, b) ((p)&~(b))
#define NEXTBLOCK(p) ((struct freehdr *)((char *)p+clearbits(p->Size)))
#define PREVFRONT(p) (((struct freetrlr *)(p)-(sizeof (struct freetrlr)/WORD))->Front)

#define RETADDROFF (6)

#ifndef IDENTIFY

struct hdr { int Size };
struct freehdr {
	int Size;
	struct freehdr *Next, *Prev;
};
struct freetrlr { struct freehdr *Front };
struct segtrlr {
	int Size;
	struct freehdr *Next, *Prev;
	struct freehdr *Front;
};

#else

/* two additional words on every free block identify the caller that created the block
   and it sequence number among all block creations */
struct hdr { 
	char *caller;
	int seqno;
	int Size;
};
struct freehdr {
	char *caller;
	int seqno;
	int Size;
	struct freehdr *Next, *Prev;
};
struct freetrlr { struct freehdr *Front; };
struct segtrlr {
	char *caller;
	int seqno;
	int Size;
	struct freehdr *Next, *Prev;
	struct freehdr *Front;
};

#endif

struct arenastate {
	struct freehdr *arenastart;
	struct freehdr *arenaend;
	struct freehdr *allocp;	/*free list ptr*/
	struct hdr *PendingFree;
	int SeqNo;
	char arenahasbeenreset;
	char InProgress;
	char RecurringM0;
	
};

struct arenastate *GetMallocArena();
char *malloc(), *realloc();

