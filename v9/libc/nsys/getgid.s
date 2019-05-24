# C library -- getgid

# gid = getgid();
#

	getgid = 47
.globl	_getgid

_getgid:
	linkw	a6,#0
	pea	getgid
	trap	#0
	unlk	a6
	rts

# C library -- getegid

# gid = getegid();
# returns effective gid

.globl	_getegid

_getegid:
	linkw	a6,#0
	pea	getgid
	trap	#0
	movl	d1,d0
	unlk	a6
	rts
