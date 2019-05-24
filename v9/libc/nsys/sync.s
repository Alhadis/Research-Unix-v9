	sync = 36
.globl	_sync

_sync:
	linkw	a6,#0
	pea	sync
	trap	#0
	unlk	a6
	rts
