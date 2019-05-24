# C library -- exit

# exit(code)
# code is return in r0 to system

	exit = 1
.globl	_exit
.globl	__cleanup

_exit:
	linkw	a6,#0
	jsr	__cleanup
	pea	exit
	trap	#0
	stop	#0
