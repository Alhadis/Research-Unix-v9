# C library -- time

# tvec = time(tvec);
#

	time = 13
.globl	_time

_time:
	linkw	a6,#0
	pea	time
	trap	#0
	movl	sp@(8),d1
	beq	nostore
	movl	d1,a1
	movl	d0,a1@
nostore:
	unlk	a6
	rts

# ftime
#
	ftime = 35
.globl	_ftime

_ftime:
	linkw	a6,#0
	pea	ftime
	trap	#0
	unlk	a6
	rts
