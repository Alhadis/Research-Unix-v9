# C library -- select

# select(nfd, rfdset, wfdset, time);
#

	select = 38
.globl	_select
.globl  cerror

_select:
	linkw	a6,#0
	pea	select
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	unlk	a6
	rts
