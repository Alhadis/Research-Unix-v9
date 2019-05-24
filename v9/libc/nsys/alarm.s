# C library - alarm, pause

	alarm = 27
.globl	_alarm
	pause = 29
.globl	_pause

_alarm:
	linkw	a6,#0
	pea	alarm
	trap	#0
	unlk	a6
	rts

_pause:
	linkw	a6,#0
	pea	pause
	trap	#0
	unlk	a6
	rts
