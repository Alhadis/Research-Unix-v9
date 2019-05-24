# C library -- getgid

# gid = getgid();
#

	getgid = 47
.globl	_getgid

_getgid:
	pea	getgid
	trap	#0
	rts

# C library -- getegid

# gid = getegid();
# returns effective gid

.globl	_getegid

_getegid:
	pea	getgid
	trap	#0
	movl	d1,d0
	rts
