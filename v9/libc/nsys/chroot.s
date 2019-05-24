#  C library -- chroot
 
#  error = chroot(string);
 
	chroot = 61
 
.globl	_chroot
.globl	cerror
_chroot:
	linkw	a6,#0
	pea	chroot
	trap	#0
	bcc	noerror
	jmp	cerror
noerror:
	unlk	a6
	rts
