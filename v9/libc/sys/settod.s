
# C library -- settod

# oldtod =  settod(newtod);

	settod = 64+42
.globl	_settod

_settod:
	pea	settod
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	rts
