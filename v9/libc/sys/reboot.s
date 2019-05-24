# C library -- reboot

# error = reboot(how);

	reboot = 55
.globl	_reboot

_reboot:
	pea	reboot
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	rts
