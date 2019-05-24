#include "process.pub"
#include "frame.pri"
#include "symtab.pri"
#include "symbol.h"
#include "m68kcore.h"
#include "asm.pri"
#include "format.pub"
#include "bpts.pri"
#include "master.pri"
SRCFILE("m68kcore.c")

char *M68kCore::read(long l,char *b,int n)	{ return readwrite(l,b,n,0); }
char *M68kCore::write(long l,char *b,int n)	{ return readwrite(l,b,0,n); }
char *M68kCore::pokedbl(long l,double d,int n)	{ return write(l,(char*)&d,n);}
int M68kCore::REG_AP()				{ return 0; }
int M68kCore::REG_FP()				{ return 14; }
int M68kCore::REG_SP()				{ return 15; }
int M68kCore::REG_PS()				{ return 16; }
int M68kCore::REG_PC()				{ return 17; }
char *M68kCore::readwrite(long, char*,int,int)	{ return "readwrite"; }
int M68kCore::instack(long, long)		{ return 1; }
int M68kCore::fpvalid(long)			{ return 1; }
Behavs M68kCore::behavetype()			{ return 0; }
char *M68kCore::dostep(long, long, int)		{ return "dostep"; }
long M68kCore::scratchaddr()			{ return 0; }
int M68kCore::nregs()				{ return 18; }
long M68kCore::regaddr()			{ return 0; }

char *M68kCore::poke(long l,long d,int n)
{
	switch (n) {
	case 1:
		return write(l, 3 + (char*)&d,n);
	case 2:
		return write(l, 2 + (char*)&d,n);
	case 4:
		return write(l,(char*)&d,n);
	default:
		return "M68kCore poke error";
	}
}

char *M68kCore::liftbpt(Trap *t)	
{
	if( behavs() == ERRORED ) return 0;
	return poke(t->stmt->range.lo, t->saved, 2);
}

#define REGBIT(r) (1 << r)
long M68kCore::saved(Frame *f, int r, int)	/* ignore size */
{
	if( r > 15 )
		return 0;
	if( !(f->regsave & REGBIT(r)) )
		return 0;
	long loc = f->regbase;
	while( --r >= 0 )
		if( f->regsave & REGBIT(r) ) loc += 4;
	return loc;
}

Asm *M68kCore::newAsm()	{ return new M68kAsm(this); }

char *M68kCore::regname(int r)
{
	static char *regnames[] = { "d0", "d1", "d2", "d3", "d4", "d5",
				   "d6", "d7", "a0", "a1", "a2", "a3",
				   "a4", "a5", "fp", "sp", "ps", "pc",
				   "usp", "isp", "vbr", "sfc", "dfc",
				   "msp", "cacr", "caar" };
	if (r < nregs())
		return regnames[r];
	else
		return 0;
}

long M68kCore::regloc(int r, int sz)
{
	if (r >= 0 && r < nregs()) {
		long ret = 4 * r + regaddr();
		if (sz && sz < 4)
			ret += 4 - sz;
		return ret;
	}
	return 0;
}


#define CYCLE 4
Cslfd *M68kCore::peek(long loc, Cslfd *fail)
{
	static i;
	static Cslfd *c;
	UCslfd u;

	if( read(loc, (char*)&u, 8) )
		return Core::peek(loc, fail);
	if( !c ) c = new Cslfd[CYCLE];
	Cslfd *p = c+(i++,i%=CYCLE);
	p->chr = u.chr;
	p->sht = u.sht;
	p->lng = u.lng;
	p->flterr = 0;
	p->flt = u.flt;
	p->dbl = u.dbl;
	return p;
}

char *M68kCore::peekstring(long loc, char *fail)
{
	static char buf[256];
	char *error = read(loc, buf, 250);
	if( error )
		return fail ? fail : strcpy(buf,error);
	return buf;
}

CallStk *M68kCore::callstack()	// do we have to peek everything twice?
{
	trace( "%d.callstack()", this );	OK(0);
	long size;
	long _fp = fp();
	if( !fpvalid(_fp))
		return (CallStk *)0;
	for( size = 1; size<1000; ++size ){
		long __fp = peek(_fp)->lng;
		if( !instack(__fp, _fp) )
			break;
		_fp = __fp;
	}
	CallStk *c = new CallStk(size, this);
	_fp = fp();
	long _pc = pc();
	for( long i = 0; i < size; ++i ){
		c->fpf[i].fp = _fp;
		c->fpf[i].func = (Func*) _symtab->loctosym(U_FUNC, _pc);
		_pc = peek(_fp+4)->lng;
		_fp = peek(_fp)->lng;
	}
	return c;
}

const short LINKA6=0x4E56, ADDLSP=0xDFFC, MOVEMLSP=0x48D7;
Frame M68kCore::frameabove(long _fp)
{
	Frame f(this);
	if( _fp ){
		f.pc = peek(_fp+4)->lng;
		f.fp = peek(_fp)->lng;
	} else {
		f.pc = pc();
		f.fp = fp();
	}
	f.ap = f.fp;
	int callpc = peek(f.fp+4)->lng;
	int inst = peek(callpc)->sht;
	f.regbase = f.fp;
	Func *funcp = (Func*)_symtab->loctosym(U_FUNC, f.pc);
	if (funcp) {
		int faddr = funcp->range.lo;
		if (peek(faddr)->sht == LINKA6) {
			f.regbase += peek(faddr+2)->sht;
			faddr += 4;
		}
		if (peek(faddr)->sht == ADDLSP) {
			f.regbase += peek(faddr+2)->lng;
			faddr += 6;
		}
		if (peek(faddr)->sht == MOVEMLSP)
			f.regsave = peek(faddr+2)->sht;
	}
	return f;
}

const short M68K_TRAP0=0x4E40, M68K_RTS = 0x4E75, M68K_UNLNKA6 = 0x4E5E;
char *M68kCore::popcallstack()
{
	long regaddr, savepc;
	char *error = 0;
	int i;
	short saveinst;
	Cslfd *c;

	if( behavetype() == ACTIVE )
		return "pop callstack: process not stopped";
	savepc = pc();
	if( peek(savepc-2)->sht == M68K_TRAP0 )
		return "pop callstack: process in system call";
	c = peek(savepc, 0);
	if( !c ) return "cannot pop callstack";
	saveinst = c->sht;
	// Must be careful if we are in the middle of the call sequence
	switch (saveinst) {
	case M68K_RTS:
		return step();
	case LINKA6:
		goto rts;
	case MOVEMLSP:
		goto unlink;
	}
	// Sometimes the space is added after the link
	if( peek(savepc-2)->sht == LINKA6 )
		goto rts;
	// Restore the registers, except the frame pointer and sp
	{ // This is here because C++ bitches about gotos if it isn't
	Frame f = frameabove(0);
	for (i = 0; i < 14; i++)
		if (regaddr = saved(&f, i, 4))
			regpoke(i, peek(regaddr)->lng);
	}
	// Pop the frame
unlink:	if( !error ) error = poke(savepc, M68K_UNLNKA6, 2);
	if( !error ) error = step();
	if( !error ) regpoke(REG_PC(), savepc);
	// Return from the subroutine
rts:	if( !error ) error = poke(savepc, M68K_RTS, 2);
	if( !error ) error = step();
	// Restore the instruction
	poke(savepc, saveinst, 2);
	return error;
}

char *M68kCore::step(long lo, long hi)
{
	return dostep(lo,hi,1);
}

const short M68K_BSR = 0x6100, M68K_JSR = 0x4e80;
const short M68K_BSR_MSK = 0xff00, M68K_JSR_MSK = 0xffc0;
char *M68kCore::stepoverM68KJSB()
{
	char *error = 0;
	static Trap *t;
	short inst;
	long fp0, offset;

	if(!t)
		t = new Trap(new Stmt(0,0,0),0);
	inst = peek(pc())->sht;
	/*
	 * Determine where to put the breakpoint depending on the
	 * addressing mode of the instruction.
	 */
	if ((inst & M68K_BSR_MSK) == M68K_BSR) {
		inst &= 0xff;
		if (inst == 0)
			offset = 4;
		else if (inst == 0xff)
			offset = 6;
		else
			offset = 2;
	} else {			/* Its a JSR */
		short reg, mode;
		reg = inst & 07;
		mode = (inst >> 3) & 0x7;
		if (mode == 7) {
			if (reg == 1)
				offset = 6;
			else if (reg == 0 || reg == 2)
				offset = 4;
			else
				goto indexed;
		} else if (mode == 2)
			offset = 2;
		else if (mode == 5)
			offset = 4;
		else {
indexed:
			inst = peek(pc()+2)->sht;
			offset = 4;
			/* Full format extension */
			if (inst & 0x0100) {
				/* Base Displacement */
				if (inst & 0x0020) {
					if (inst & 0x0010)
						offset += 4;
					else
						offset += 2;
				}
				/* Outer Displacement */
				if (inst & 0x0002) {
					if (inst & 0x0001)
						offset += 4;
					else
						offset += 2;
				}
			}
		}

	}
	t->stmt->range.lo = pc()+offset;
	fp0 = fp();
	if(error = laybpt(t))
		return error;
	for(;;) {
		error = dostep(0,0,0);
		if (error || fp() >= fp0)
			break;
		if( error = liftbpt(t) ) return error;
		if( error = dostep(0,0,1) ) return error;
		if( error = laybpt(t) ) return error;
	}
	if( !error )
		error = liftbpt(t);
	else
		liftbpt(t);
	return error;
}

int M68kCore::isM68KJSB(int inst)
{
	return (inst & M68K_BSR_MSK) == M68K_BSR ||
	       (inst & M68K_JSR_MSK) == M68K_JSR;
}

char *M68kCore::stepprolog()
{
	Func *f = (Func*)_symtab->loctosym(U_FUNC, pc());
	if (f) {
		// Only step again if we are in the function prolog
		Stmt *s = f->stmt(pc());
		if (s && s->range.lo == f->range.lo)
			process()->stmtstep(1);
	}
	return 0;
}

char *M68kCore::docall(long addr, int numarg)
{
	const int CALL_SIZE=12, JSR=0x4eb9, ADDSP=0xdffc;
	char save[CALL_SIZE], *error;
	int i;

	if( behavetype() == ACTIVE )
		return "process not stopped";
	if( !online() )
		return "cannot restart dump";
	int callstart = scratchaddr();
	for( i = 0; i < CALL_SIZE; ++i )
		save[i] = peek(callstart+i)->chr;
	if( ( error = poke(callstart+0, JSR, 2) )
	 || ( error = poke(callstart+2, addr, 4) )
	 || ( error = poke(callstart+6, ADDSP, 2) )
	 || ( error = poke(callstart+8, numarg * 4, 4) ) )
		return error;
	if( ( error = regpoke(REG_PC(), callstart) )
	 || ( error = step( callstart, callstart+CALL_SIZE) ) )
		return error;
	for( i = 0; i < CALL_SIZE; ++i )
		if( error = poke(callstart+i, save[i], 1 ) )
			return error;
	return 0;
}

long M68kCore::apforcall(int argbytes)
{
	regpoke(REG_SP(), sp() - argbytes);
	return sp() - 8;
}

long M68kCore::returnregloc()
{
	return regloc(0);
}

Context* M68kCore::newContext()
{
	M68kContext *cc = new M68kContext;
	cc->error = 0;
	cc->core = this;
	if( behavetype() == ACTIVE )
		cc->error = "context save: process not stopped";
	if( !cc->error )
		cc->error = read(regaddr(),(char*)cc->regs,sizeof(cc->regs));
	return cc;
}

void M68kContext::restore()
{
	error = core->write(core->regaddr(), (char*)regs, sizeof(regs));
}
