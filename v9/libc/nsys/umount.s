# C library -- umount/

	umount = 22
.globl	_umount
.globl	cerror
.comm	_errno,4

_umount:
	linkw	a6,#0
	pea	umount
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	unlk	a6
	rts
