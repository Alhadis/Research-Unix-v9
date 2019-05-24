# C library -- link

# error = link(old-file, new-file);
#

	link = 9
.globl	_link
.globl cerror

_link:
	linkw	a6,#0
	pea	link
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	unlk	a6
	rts
