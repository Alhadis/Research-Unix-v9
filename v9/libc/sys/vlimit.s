# C library -- vlimit

# oldvalue = vlimit(what, newvalue);
# if newvalue == -1 old value is returned and the limit is not changed

	vlimit = 64+13
.globl	_vlimit

_vlimit:
	pea	vlimit
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	rts
