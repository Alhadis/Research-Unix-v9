# C library -- getuid

# uid = getuid();
#

	getuid = 24
.globl	_getuid

_getuid:
	linkw	a6,#0
	pea	getuid
	trap	#0
	unlk	a6
	rts



# C library -- geteuid

# uid = geteuid();
#  returns effective uid

.globl	_geteuid

_geteuid:
	linkw	a6,#0
	pea	getuid
	trap	#0
	movl	d1,d0
	unlk	a6
	rts
