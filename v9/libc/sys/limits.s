# C library -- limits

# Control limit structures

# limits(lp, select)
# struct lnode *lp;
# int select;

#  0 - return own limit struct in *lnode
#  1 - return limit struct of lnode->l_uid in *lnode
#  2 - return all active limit structs in array lnode
#  3 - (root only) set up new limit struct from *lnode
#  4 - (init only) wait for a dead process, and return limit, proc struct
#  5 - (root only) change limits fields in active lnode
#  6 - (root only) get dead group's limit struct

	limits = 127

.globl	_limits
.globl	cerror

_limits:
	pea	limits
	trap	#0
	bcs 	error
	rts
error:
	jmp 	cerror
