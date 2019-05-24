# C library -- vlimit

# oldvalue = vlimit(what, newvalue);
# if newvalue == -1 old value is returned and the limit is not changed

	vlimit = 64+13
.globl	_vlimit

_vlimit:
	linkw	a6,#0
	pea	vlimit
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	unlk	a6
	rts
