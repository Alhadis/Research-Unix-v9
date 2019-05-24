# C library -- utime
 
#  error = utime(string,timev);
 
.globl	_utime
.globl	cerror
	utime = 30
 
_utime:
	pea	utime
	trap	#0
	bcc	noerror
	jmp	cerror
noerror:
	rts
