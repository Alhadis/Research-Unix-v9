# profil

	prof = 44
.globl	_profil
_profil:
	pea	prof
	trap	#0
	rts
