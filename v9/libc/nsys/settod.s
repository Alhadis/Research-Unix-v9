
# C library -- settod

# oldtod =  settod(newtod);

	settod = 64+42
.globl	_settod

_settod:
	linkw	a6,#0
	pea	settod
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	unlk	a6
	rts
