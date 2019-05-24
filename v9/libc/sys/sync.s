	sync = 36
.globl	_sync

_sync:
	pea	sync
	trap	#0
	rts
