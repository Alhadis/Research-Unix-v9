# C library -- reboot

# error = reboot(how);

	reboot = 55
.globl	_reboot

_reboot:
	linkw	a6,#0
	pea	reboot
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	unlk	a6
	rts
