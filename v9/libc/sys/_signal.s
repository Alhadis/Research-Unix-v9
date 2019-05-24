# C library -- _signal

# _signal(sig, func)
# User calls signal whiches places trampoline code to be called on a
# signal.
	signal = 48
.globl	__signal
.globl  cerror2

__signal:
	pea	signal
	trap	#0
	bcc 	noerr
	jmp 	cerror2
noerr:
	movl	d0,a0
	rts
