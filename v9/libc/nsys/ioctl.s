#  C library -- ioctl
 
#  ioctl(fdes,command,arg)
#  struct * arg;
#
#  result == -1 if error
 
	ioctl = 54
.globl	_ioctl
.globl cerror
 
_ioctl:
	linkw	a6,#0
	pea	ioctl
	trap	#0
	bcc	noerror
	jmp	cerror
noerror:
	unlk	a6
	rts
