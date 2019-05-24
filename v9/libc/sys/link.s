# C library -- link

# error = link(old-file, new-file);
#

	link = 9
.globl	_link
.globl cerror

_link:
	pea	link
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	rts
