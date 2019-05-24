# C library -- getuid

# uid = getuid();
#

	getuid = 24
.globl	_getuid

_getuid:
	pea	getuid
	trap	#0
	rts



# C library -- geteuid

# uid = geteuid();
#  returns effective uid

.globl	_geteuid

_geteuid:
	pea	getuid
	trap	#0
	movl	d1,d0
	rts
