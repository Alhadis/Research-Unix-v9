# profil

	prof = 44
.globl	_profil
_profil:
	linkw	a6,#0
	pea	prof
	trap	#0
	unlk	a6
	rts
