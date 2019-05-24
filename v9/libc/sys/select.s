# C library -- select

# select(nfd, rfdset, wfdset, time);
#

	select = 38
.globl	_select
.globl  cerror

_select:
	pea	select
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	rts
