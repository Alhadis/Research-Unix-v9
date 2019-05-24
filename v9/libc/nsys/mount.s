# C library -- mount

# error = mount(dev, file, flag)

	mount = 21
.globl	_mount
.globl  cerror

_mount:
	linkw	a6,#0
	pea	mount
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	unlk	a6
	rts
