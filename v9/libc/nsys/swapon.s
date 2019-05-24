# C library -- swapon

# error = swapon(string);

	swapon = 64+21
.globl	_swapon

_swapon:
	linkw	a6,#0
	pea	swapon
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	unlk	a6
	rts
