# C library -- _exit

# _exit(code)
# code is return in r0 to system
# Same as plain exit, for user who want to define their own exit.

	exit = 1
.globl	__exit

__exit:
	pea	exit
	trap	#0
	stop	#0
