# C library -- umount/

	umount = 22
.globl	_umount
.globl	cerror
.comm	_errno,4

_umount:
	pea	umount
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	rts
