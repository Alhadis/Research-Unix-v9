# C library -- setpgrp, getpgrp

# setpgrp(pid, pgrp);	/* set pgrp of pid and descendants to pgrp */
# if pid==0 use current pid
#
# getpgrp(pid)
# implemented as setpgrp(pid, -1)

	setpgrp = 39
.globl	_setpgrp
.globl	_getpgrp
.globl  cerror

_setpgrp:
	linkw	a6,#0
	pea	setpgrp
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	unlk	a6
	rts

_getpgrp:
	pea	-1
	movl	sp@(8),sp@-
	jsr	gpgrp
	addql	#8,sp
	rts
gpgrp:
	linkw	a6,#0
	pea	setpgrp
	trap	#0
	bcc	noerror
	jmp	cerror
