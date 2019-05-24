#  C library -- chroot
 
#  error = chroot(string);
 
	chroot = 61
 
.globl	_chroot
.globl	cerror
_chroot:
	pea	chroot
	trap	#0
	bcc	noerror
	jmp	cerror
noerror:
	rts
