# C library -- time

# tvec = time(tvec);
#

	time = 13
.globl	_time

_time:
	pea	time
	trap	#0
	movl	sp@(4),d1
	beq	nostore
	movl	d1,a1
	movl	d0,a1@
nostore:
	rts

# ftime
#
	ftime = 35
.globl	_ftime

_ftime:
	pea	ftime
	trap	#0
	rts
