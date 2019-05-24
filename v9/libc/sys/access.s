# access(file, request)
#  test ability to access file in all indicated ways
#  1 - read
#  2 - write
#  4 - execute

	access = 33
.globl	_access

_access:
	pea	access
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	rts
