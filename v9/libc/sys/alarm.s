# C library - alarm, pause

	alarm = 27
.globl	_alarm
	pause = 29
.globl	_pause

_alarm:
	pea	alarm
	trap	#0
	rts

_pause:
	pea	pause
	trap	#0
	rts
