# C library -- execve

# execve(file, argv, env);
#
# where argv is a vector argv[0] ... argv[x], 0
# last vector element must be 0

	exece = 59
.globl	_execve

_execve:
	pea	exece
	trap	#0
	jmp 	cerror
