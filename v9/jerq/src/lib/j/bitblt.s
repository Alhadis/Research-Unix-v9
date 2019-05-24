	.file	"bitblt.c"
	.data
	.text
	.align	4
	.def	bitblt;	.val	bitblt;	.scl	2;	.type	044;	.endef
	.globl	bitblt
bitblt:
	save	&.R1
	addw2	&.F1,%sp
#	line 38, file "bitblt.c"
	addw3	&8,0(%ap),%r0
	cmph	4(%ap),0(%r0)
	jge	.L32
#	line 39, file "bitblt.c"
	addw3	&8,0(%ap),%r0
	movh	0(%r0),4(%ap)
.L32:
#	line 40, file "bitblt.c"
	addw3	&12,0(%ap),%r0
	cmph	8(%ap),0(%r0)
	jle	.L33
#	line 41, file "bitblt.c"
	addw3	&12,0(%ap),%r0
	movh	0(%r0),8(%ap)
.L33:
#	line 42, file "bitblt.c"
	addw3	&10,0(%ap),%r0
	cmph	6(%ap),0(%r0)
	jge	.L34
#	line 43, file "bitblt.c"
	addw3	&10,0(%ap),%r0
	movh	0(%r0),6(%ap)
.L34:
#	line 44, file "bitblt.c"
	addw3	&14,0(%ap),%r0
	cmph	10(%ap),0(%r0)
	jle	.L35
#	line 45, file "bitblt.c"
	addw3	&14,0(%ap),%r0
	movh	0(%r0),10(%ap)
.L35:
#	line 52, file "bitblt.c"
	addw3	&8,12(%ap),%r0
	cmph	16(%ap),0(%r0)
	jge	.L36
#	line 53, file "bitblt.c"
	addw3	&8,12(%ap),%r0
	subh3	16(%ap),0(%r0),%r0
	addh2	%r0,4(%ap)
#	line 54, file "bitblt.c"
	addw3	&8,12(%ap),%r0
	movh	0(%r0),16(%ap)
.L36:
#	line 56, file "bitblt.c"
	addw3	&10,12(%ap),%r0
	cmph	18(%ap),0(%r0)
	jge	.L37
#	line 57, file "bitblt.c"
	addw3	&10,12(%ap),%r0
	subh3	18(%ap),0(%r0),%r0
	addh2	%r0,6(%ap)
#	line 58, file "bitblt.c"
	addw3	&10,12(%ap),%r0
	movh	0(%r0),18(%ap)
.L37:
#	line 60, file "bitblt.c"
	subh3	4(%ap),8(%ap),%r0
	addw3	&12,12(%ap),%r1
	subh3	16(%ap),0(%r1),%r1
	cmpw	%r0,%r1
	jle	.L38
#	line 61, file "bitblt.c"
	addw3	&12,12(%ap),%r0
	subh3	16(%ap),0(%r0),%r0
	addh2	4(%ap),%r0
	movh	%r0,8(%ap)
.L38:
#	line 62, file "bitblt.c"
	subh3	6(%ap),10(%ap),%r0
	addw3	&14,12(%ap),%r1
	subh3	18(%ap),0(%r1),%r1
	cmpw	%r0,%r1
	jle	.L39
#	line 63, file "bitblt.c"
	addw3	&14,12(%ap),%r0
	subh3	18(%ap),0(%r0),%r0
	addh2	6(%ap),%r0
	movh	%r0,10(%ap)
.L39:
#	line 64, file "bitblt.c"
	subh3	6(%ap),10(%ap),%r0
	movw	%r0,%r3
#	line 65, file "bitblt.c"
	subh3	4(%ap),8(%ap),%r0
	subw2	&1,%r0
	movw	%r0,%r5
#	line 66, file "bitblt.c"
	cmpw	%r3,&0
	jle	.L41
	cmpw	%r5,&0
	jge	.L40
.L41:
	jmp	.L31
.L40:
#	line 68, file "bitblt.c"
	cmpw	%r5,&32
	jge	.L42
	jmp	.L43
.L42:
#	line 70, file "bitblt.c"
	movw	%r5,%r4
#	line 71, file "bitblt.c"
	movw	%r3,12(%fp)
#	line 72, file "bitblt.c"
	andh3	&31,16(%ap),%r0
	LLSW3	&2,%r0,%r0
	movw	topbits(%r0),48(%fp)
#	line 73, file "bitblt.c"
	mcomw	48(%fp),%r0
	movw	%r0,32(%fp)
#	line 74, file "bitblt.c"
	movbhw	16(%ap),%r0
	addw2	%r4,%r0
	andw2	&31,%r0
	addw2	&1,%r0
	LLSW3	&2,%r0,%r0
	movw	topbits(%r0),36(%fp)
#	line 75, file "bitblt.c"
	mcomw	36(%fp),%r0
	movw	%r0,52(%fp)
#	line 76, file "bitblt.c"
	movbhw	16(%ap),%r0
	addw2	%r4,%r0
	LRSW3	&5,%r0,%r0
	movbhw	16(%ap),%r1
	LRSW3	&5,%r1,%r1
	subw2	%r1,%r0
	movw	%r0,16(%fp)
#	line 77, file "bitblt.c"
	addw3	&4,0(%ap),%r0
	subw3	16(%fp),0(%r0),%r0
	LLSW3	&2,%r0,%r0
	movw	%r0,%r6
#	line 78, file "bitblt.c"
	addw3	&4,12(%ap),%r0
	subw3	16(%fp),0(%r0),%r0
	LLSW3	&2,%r0,%r0
	movw	%r0,%r5
#	line 79, file "bitblt.c"
	cmpw	0(%ap),12(%ap)
	jne	.L44
#	line 80, file "bitblt.c"
	cmph	6(%ap),18(%ap)
	jge	.L45
#	line 81, file "bitblt.c"
	movtwh	%r3,%r0
	subh2	&1,%r0
	addh2	%r0,6(%ap)
#	line 82, file "bitblt.c"
	movtwh	%r3,%r0
	subh2	&1,%r0
	addh2	%r0,18(%ap)
#	line 83, file "bitblt.c"
	cmph	4(%ap),16(%ap)
	jge	.L46
#	line 84, file "bitblt.c"
	orw2	&8,20(%ap)
#	line 85, file "bitblt.c"
	movtwh	%r4,%r0
	addh2	4(%ap),%r0
	movh	%r0,4(%ap)
#	line 86, file "bitblt.c"
	movtwh	%r4,%r0
	addh2	16(%ap),%r0
	movh	%r0,16(%ap)
#	line 87, file "bitblt.c"
	mnegw	%r6,%r0
	movw	%r0,%r6
#	line 88, file "bitblt.c"
	mnegw	%r5,%r0
	movw	%r0,%r5
	jmp	.L47
.L46:
#	line 92, file "bitblt.c"
	addw3	&4,0(%ap),%r0
	LLSW3	&3,0(%r0),%r0
	subw2	%r0,%r6
#	line 93, file "bitblt.c"
	addw3	&4,12(%ap),%r0
	LLSW3	&3,0(%r0),%r0
	subw2	%r0,%r5
.L47:
	jmp	.L48
.L45:
#	line 98, file "bitblt.c"
	cmph	4(%ap),16(%ap)
	jge	.L49
#	line 99, file "bitblt.c"
	orw2	&8,20(%ap)
#	line 100, file "bitblt.c"
	movtwh	%r4,%r0
	addh2	4(%ap),%r0
	movh	%r0,4(%ap)
#	line 101, file "bitblt.c"
	movtwh	%r4,%r0
	addh2	16(%ap),%r0
	movh	%r0,16(%ap)
#	line 102, file "bitblt.c"
	addw3	&4,0(%ap),%r0
	addw3	16(%fp),0(%r0),%r0
	LLSW3	&2,%r0,%r0
	movw	%r0,%r6
#	line 103, file "bitblt.c"
	addw3	&4,12(%ap),%r0
	addw3	16(%fp),0(%r0),%r0
	LLSW3	&2,%r0,%r0
	movw	%r0,%r5
.L49:
.L48:
.L44:
#	line 107, file "bitblt.c"
	subw2	&1,16(%fp)
#	line 108, file "bitblt.c"
	andh3	&31,16(%ap),%r0
	movw	%r0,24(%fp)
#	line 109, file "bitblt.c"
	andh3	&31,4(%ap),%r0
	movw	%r0,28(%fp)
#	line 110, file "bitblt.c"
	pushw	12(%ap)
	pushw	16(%ap)
	call	&2,addr
	movw	%r0,%r7
#	line 111, file "bitblt.c"
	pushw	0(%ap)
	pushw	4(%ap)
	call	&2,addr
	movw	%r0,%r8
#	line 112, file "bitblt.c"
	subw3	28(%fp),24(%fp),%r0
	movw	%r0,0(%fp)
#	line 113, file "bitblt.c"
	cmpw	0(%fp),&0
	jne	.L50
#	line 114, file "bitblt.c"
	orw2	&4,20(%ap)
	jmp	.L51
.L50:
#	line 115, file "bitblt.c"
	cmpw	0(%fp),&0
	jge	.L52
#	line 116, file "bitblt.c"
	addw2	&32,0(%fp)
.L52:
.L51:
#	line 118, file "bitblt.c"
	subw3	0(%fp),&32,%r0
	movw	%r0,4(%fp)
#	line 119, file "bitblt.c"
	movw	20(%ap),%r0
	jmp	.L54
.L55:
#	line 122, file "bitblt.c"
	LRSW3	&2,16(%fp),%r0
	movw	%r0,4(%fp)
#	line 123, file "bitblt.c"
	andw3	&3,16(%fp),%r0
	movw	%r0,16(%fp)
#	line 124, file "bitblt.c"
	movw	12(%fp),%r4
.L58:
#	line 126, file "bitblt.c"
	movw	%r7,%r0
	subw2	&4,%r7
	movw	%r8,%r1
	subw2	&4,%r8
	andw3	0(%r1),36(%fp),%r1
	orw2	%r1,0(%r0)
#	line 127, file "bitblt.c"
	movw	4(%fp),%r3
	jnpos	.L59
.L62:
#	line 128, file "bitblt.c"
	orw2	0(%r8),0(%r7)
#	line 129, file "bitblt.c"
	orw2	-4(%r8),-4(%r7)
#	line 130, file "bitblt.c"
	orw2	-8(%r8),-8(%r7)
#	line 131, file "bitblt.c"
	orw2	-12(%r8),-12(%r7)
#	line 132, file "bitblt.c"
	subw2	&16,%r7
#	line 133, file "bitblt.c"
	subw2	&16,%r8
.L61:
#	line 134, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L62
.L60:
.L59:
#	line 135, file "bitblt.c"
	movw	16(%fp),%r3
	jnpos	.L63
.L66:
#	line 136, file "bitblt.c"
	movw	%r7,%r0
	subw2	&4,%r7
	movw	%r8,%r1
	subw2	&4,%r8
	orw2	0(%r1),0(%r0)
.L65:
#	line 137, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L66
.L64:
.L63:
#	line 138, file "bitblt.c"
	andw3	0(%r8),32(%fp),%r0
	orw2	%r0,0(%r7)
 ADDW2	%r6,%r8
 ADDW2	%r5,%r7
.L57:
#	line 141, file "bitblt.c"
	subw2	&1,%r4
	jnz	.L58
.L56:
	jmp	.L53
.L67:
#	line 144, file "bitblt.c"
	cmpw	24(%fp),28(%fp)
	jge	.L68
#	line 145, file "bitblt.c"
	addw2	&4,%r8
.L68:
.L71:
#	line 147, file "bitblt.c"
	movw	%r8,%r0
	subw2	&4,%r8
	movw	0(%r0),%r4
#	line 148, file "bitblt.c"
	movw	%r7,%r0
	subw2	&4,%r7
	LRSW3	0(%fp),%r4,%r1
	LLSW3	4(%fp),0(%r8),%r2
	orw2	%r2,%r1
	andw2	36(%fp),%r1
	orw2	%r1,0(%r0)
#	line 149, file "bitblt.c"
	movw	16(%fp),%r3
	jnpos	.L72
.L75:
#	line 150, file "bitblt.c"
	movw	%r8,%r0
	subw2	&4,%r8
	LRSW3	0(%fp),0(%r0),%r0
	movw	%r0,%r4
#	line 151, file "bitblt.c"
	movw	%r7,%r0
	subw2	&4,%r7
	LLSW3	4(%fp),0(%r8),%r1
	orw2	%r4,%r1
	orw2	%r1,0(%r0)
.L74:
#	line 152, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L75
.L73:
.L72:
#	line 153, file "bitblt.c"
	movw	0(%r8),%r4
#	line 154, file "bitblt.c"
	LRSW3	0(%fp),%r4,%r0
	LLSW3	4(%fp),-4(%r8),%r1
	orw2	%r1,%r0
	andw2	32(%fp),%r0
	orw2	%r0,0(%r7)
 addw2	%r6,%r8
 addw2	%r5,%r7
.L70:
#	line 157, file "bitblt.c"
	subw2	&1,12(%fp)
	jpos	.L71
.L69:
	jmp	.L53
.L76:
#	line 160, file "bitblt.c"
	LRSW3	&2,16(%fp),%r0
	movw	%r0,4(%fp)
#	line 161, file "bitblt.c"
	andw3	&3,16(%fp),%r0
	movw	%r0,16(%fp)
#	line 162, file "bitblt.c"
	movw	12(%fp),%r4
.L79:
#	line 164, file "bitblt.c"
	movw	%r7,%r0
	addw2	&4,%r7
	movw	%r8,%r1
	addw2	&4,%r8
	andw3	0(%r1),32(%fp),%r1
	orw2	%r1,0(%r0)
#	line 165, file "bitblt.c"
	movw	4(%fp),%r3
	jnpos	.L80
.L83:
#	line 166, file "bitblt.c"
	orw2	0(%r8),0(%r7)
#	line 167, file "bitblt.c"
	orw2	4(%r8),4(%r7)
#	line 168, file "bitblt.c"
	orw2	8(%r8),8(%r7)
#	line 169, file "bitblt.c"
	orw2	12(%r8),12(%r7)
#	line 170, file "bitblt.c"
	addw2	&16,%r7
#	line 171, file "bitblt.c"
	addw2	&16,%r8
.L82:
#	line 172, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L83
.L81:
.L80:
#	line 173, file "bitblt.c"
	movw	16(%fp),%r3
	jnpos	.L84
.L87:
#	line 174, file "bitblt.c"
	movw	%r7,%r0
	addw2	&4,%r7
	movw	%r8,%r1
	addw2	&4,%r8
	orw2	0(%r1),0(%r0)
.L86:
#	line 175, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L87
.L85:
.L84:
#	line 176, file "bitblt.c"
	andw3	0(%r8),36(%fp),%r0
	orw2	%r0,0(%r7)
 addw2	%r6,%r8
 addw2	%r5,%r7
.L78:
#	line 179, file "bitblt.c"
	subw2	&1,%r4
	jnz	.L79
.L77:
	jmp	.L53
.L88:
#	line 182, file "bitblt.c"
	cmpw	24(%fp),28(%fp)
	jle	.L89
#	line 183, file "bitblt.c"
	subw2	&4,%r8
.L89:
.L92:
#	line 185, file "bitblt.c"
	movw	%r8,%r0
	addw2	&4,%r8
	movw	0(%r0),%r4
#	line 186, file "bitblt.c"
	movw	%r7,%r0
	addw2	&4,%r7
	LLSW3	4(%fp),%r4,%r1
	LRSW3	0(%fp),0(%r8),%r2
	orw2	%r2,%r1
	andw2	32(%fp),%r1
	orw2	%r1,0(%r0)
#	line 187, file "bitblt.c"
	movw	16(%fp),%r3
	jnpos	.L93
.L96:
#	line 188, file "bitblt.c"
	movw	%r8,%r0
	addw2	&4,%r8
	LLSW3	4(%fp),0(%r0),%r0
	movw	%r0,%r4
#	line 189, file "bitblt.c"
	movw	%r7,%r0
	addw2	&4,%r7
	LRSW3	0(%fp),0(%r8),%r1
	orw2	%r4,%r1
	orw2	%r1,0(%r0)
.L95:
#	line 190, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L96
.L94:
.L93:
#	line 191, file "bitblt.c"
	movw	0(%r8),%r4
#	line 192, file "bitblt.c"
	LLSW3	4(%fp),%r4,%r0
	LRSW3	0(%fp),4(%r8),%r1
	orw2	%r1,%r0
	andw2	36(%fp),%r0
	orw2	%r0,0(%r7)
 addw2	%r6,%r8
 addw2	%r5,%r7
.L91:
#	line 195, file "bitblt.c"
	subw2	&1,12(%fp)
	jpos	.L92
.L90:
	jmp	.L53
.L97:
#	line 198, file "bitblt.c"
	LRSW3	&2,16(%fp),%r0
	movw	%r0,4(%fp)
#	line 199, file "bitblt.c"
	andw3	&3,16(%fp),%r0
	movw	%r0,16(%fp)
#	line 200, file "bitblt.c"
	movw	12(%fp),%r4
.L100:
#	line 202, file "bitblt.c"
	movw	%r7,%r0
	subw2	&4,%r7
	movw	%r8,%r1
	subw2	&4,%r8
	andw3	0(%r1),36(%fp),%r1
	mcomw	%r1,%r1
	andw2	%r1,0(%r0)
#	line 203, file "bitblt.c"
	movw	4(%fp),%r3
	jnpos	.L101
.L104:
#	line 204, file "bitblt.c"
	mcomw	0(%r8),%r0
	andw2	%r0,0(%r7)
#	line 205, file "bitblt.c"
	mcomw	-4(%r8),%r0
	andw2	%r0,-4(%r7)
#	line 206, file "bitblt.c"
	mcomw	-8(%r8),%r0
	andw2	%r0,-8(%r7)
#	line 207, file "bitblt.c"
	mcomw	-12(%r8),%r0
	andw2	%r0,-12(%r7)
#	line 208, file "bitblt.c"
	subw2	&16,%r7
#	line 209, file "bitblt.c"
	subw2	&16,%r8
.L103:
#	line 210, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L104
.L102:
.L101:
#	line 211, file "bitblt.c"
	movw	16(%fp),%r3
	jnpos	.L105
.L108:
#	line 212, file "bitblt.c"
	movw	%r7,%r0
	subw2	&4,%r7
	movw	%r8,%r1
	subw2	&4,%r8
	mcomw	0(%r1),%r1
	andw2	%r1,0(%r0)
.L107:
#	line 213, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L108
.L106:
.L105:
#	line 214, file "bitblt.c"
	andw3	0(%r8),32(%fp),%r0
	mcomw	%r0,%r0
	andw2	%r0,0(%r7)
 addw2	%r6,%r8
 addw2	%r5,%r7
.L99:
#	line 217, file "bitblt.c"
	subw2	&1,%r4
	jnz	.L100
.L98:
	jmp	.L53
.L109:
#	line 220, file "bitblt.c"
	cmpw	24(%fp),28(%fp)
	jge	.L110
#	line 221, file "bitblt.c"
	addw2	&4,%r8
.L110:
.L113:
#	line 223, file "bitblt.c"
	movw	%r8,%r0
	subw2	&4,%r8
	movw	0(%r0),%r4
#	line 224, file "bitblt.c"
	movw	%r7,%r0
	subw2	&4,%r7
	LRSW3	0(%fp),%r4,%r1
	LLSW3	4(%fp),0(%r8),%r2
	orw2	%r2,%r1
	andw2	36(%fp),%r1
	mcomw	%r1,%r1
	andw2	%r1,0(%r0)
#	line 225, file "bitblt.c"
	movw	16(%fp),%r3
	jnpos	.L114
.L117:
#	line 226, file "bitblt.c"
	movw	%r8,%r0
	subw2	&4,%r8
	LRSW3	0(%fp),0(%r0),%r0
	movw	%r0,%r4
#	line 227, file "bitblt.c"
	movw	%r7,%r0
	subw2	&4,%r7
	LLSW3	4(%fp),0(%r8),%r1
	orw2	%r4,%r1
	mcomw	%r1,%r1
	andw2	%r1,0(%r0)
.L116:
#	line 228, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L117
.L115:
.L114:
#	line 229, file "bitblt.c"
	movw	0(%r8),%r4
#	line 230, file "bitblt.c"
	LRSW3	0(%fp),%r4,%r0
	LLSW3	4(%fp),-4(%r8),%r1
	orw2	%r1,%r0
	andw2	32(%fp),%r0
	mcomw	%r0,%r0
	andw2	%r0,0(%r7)
 addw2	%r6,%r8
 addw2	%r5,%r7
.L112:
#	line 233, file "bitblt.c"
	subw2	&1,12(%fp)
	jpos	.L113
.L111:
	jmp	.L53
.L118:
#	line 236, file "bitblt.c"
	LRSW3	&2,16(%fp),%r0
	movw	%r0,4(%fp)
#	line 237, file "bitblt.c"
	andw3	&3,16(%fp),%r0
	movw	%r0,16(%fp)
#	line 238, file "bitblt.c"
	movw	12(%fp),%r4
.L121:
#	line 240, file "bitblt.c"
	movw	%r7,%r0
	addw2	&4,%r7
	movw	%r8,%r1
	addw2	&4,%r8
	andw3	0(%r1),32(%fp),%r1
	mcomw	%r1,%r1
	andw2	%r1,0(%r0)
#	line 241, file "bitblt.c"
	movw	4(%fp),%r3
	jnpos	.L122
.L125:
#	line 242, file "bitblt.c"
	mcomw	0(%r8),%r0
	andw2	%r0,0(%r7)
#	line 243, file "bitblt.c"
	mcomw	4(%r8),%r0
	andw2	%r0,4(%r7)
#	line 244, file "bitblt.c"
	mcomw	8(%r8),%r0
	andw2	%r0,8(%r7)
#	line 245, file "bitblt.c"
	mcomw	12(%r8),%r0
	andw2	%r0,12(%r7)
#	line 246, file "bitblt.c"
	addw2	&16,%r7
#	line 247, file "bitblt.c"
	addw2	&16,%r8
.L124:
#	line 248, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L125
.L123:
.L122:
#	line 249, file "bitblt.c"
	movw	16(%fp),%r3
	jnpos	.L126
.L129:
#	line 250, file "bitblt.c"
	movw	%r7,%r0
	addw2	&4,%r7
	movw	%r8,%r1
	addw2	&4,%r8
	mcomw	0(%r1),%r1
	andw2	%r1,0(%r0)
.L128:
#	line 251, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L129
.L127:
.L126:
#	line 252, file "bitblt.c"
	andw3	0(%r8),36(%fp),%r0
	mcomw	%r0,%r0
	andw2	%r0,0(%r7)
 addw2	%r6,%r8
 addw2	%r5,%r7
.L120:
#	line 255, file "bitblt.c"
	subw2	&1,%r4
	jnz	.L121
.L119:
	jmp	.L53
.L130:
#	line 258, file "bitblt.c"
	cmpw	24(%fp),28(%fp)
	jle	.L131
#	line 259, file "bitblt.c"
	subw2	&4,%r8
.L131:
.L134:
#	line 261, file "bitblt.c"
	movw	%r8,%r0
	addw2	&4,%r8
	movw	0(%r0),%r4
#	line 262, file "bitblt.c"
	movw	%r7,%r0
	addw2	&4,%r7
	LLSW3	4(%fp),%r4,%r1
	LRSW3	0(%fp),0(%r8),%r2
	orw2	%r2,%r1
	andw2	32(%fp),%r1
	mcomw	%r1,%r1
	andw2	%r1,0(%r0)
#	line 263, file "bitblt.c"
	movw	16(%fp),%r3
	jnpos	.L135
.L138:
#	line 264, file "bitblt.c"
	movw	%r8,%r0
	addw2	&4,%r8
	LLSW3	4(%fp),0(%r0),%r0
	movw	%r0,%r4
#	line 265, file "bitblt.c"
	movw	%r7,%r0
	addw2	&4,%r7
	LRSW3	0(%fp),0(%r8),%r1
	orw2	%r4,%r1
	mcomw	%r1,%r1
	andw2	%r1,0(%r0)
.L137:
#	line 266, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L138
.L136:
.L135:
#	line 267, file "bitblt.c"
	movw	0(%r8),%r4
#	line 268, file "bitblt.c"
	LLSW3	4(%fp),%r4,%r0
	LRSW3	0(%fp),4(%r8),%r1
	orw2	%r1,%r0
	andw2	36(%fp),%r0
	mcomw	%r0,%r0
	andw2	%r0,0(%r7)
 addw2	%r6,%r8
 addw2	%r5,%r7
.L133:
#	line 271, file "bitblt.c"
	subw2	&1,12(%fp)
	jpos	.L134
.L132:
	jmp	.L53
.L139:
#	line 274, file "bitblt.c"
	LRSW3	&2,16(%fp),%r0
	movw	%r0,4(%fp)
#	line 275, file "bitblt.c"
	andw3	&3,16(%fp),%r0
	movw	%r0,16(%fp)
#	line 276, file "bitblt.c"
	movw	12(%fp),%r4
.L142:
#	line 278, file "bitblt.c"
	movw	%r7,%r0
	subw2	&4,%r7
	movw	%r8,%r1
	subw2	&4,%r8
	andw3	0(%r1),36(%fp),%r1
	xorw2	%r1,0(%r0)
#	line 279, file "bitblt.c"
	movw	4(%fp),%r3
	jnpos	.L143
.L146:
#	line 280, file "bitblt.c"
	xorw2	0(%r8),0(%r7)
#	line 281, file "bitblt.c"
	xorw2	-4(%r8),-4(%r7)
#	line 282, file "bitblt.c"
	xorw2	-8(%r8),-8(%r7)
#	line 283, file "bitblt.c"
	xorw2	-12(%r8),-12(%r7)
#	line 284, file "bitblt.c"
	subw2	&16,%r7
#	line 285, file "bitblt.c"
	subw2	&16,%r8
.L145:
#	line 286, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L146
.L144:
.L143:
#	line 287, file "bitblt.c"
	movw	16(%fp),%r3
	jnpos	.L147
.L150:
#	line 288, file "bitblt.c"
	movw	%r7,%r0
	subw2	&4,%r7
	movw	%r8,%r1
	subw2	&4,%r8
	xorw2	0(%r1),0(%r0)
.L149:
#	line 289, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L150
.L148:
.L147:
#	line 290, file "bitblt.c"
	andw3	0(%r8),32(%fp),%r0
	xorw2	%r0,0(%r7)
 addw2	%r6,%r8
 addw2	%r5,%r7
.L141:
#	line 293, file "bitblt.c"
	subw2	&1,%r4
	jnz	.L142
.L140:
	jmp	.L53
.L151:
#	line 296, file "bitblt.c"
	cmpw	24(%fp),28(%fp)
	jge	.L152
#	line 297, file "bitblt.c"
	addw2	&4,%r8
.L152:
.L155:
#	line 299, file "bitblt.c"
	movw	%r8,%r0
	subw2	&4,%r8
	movw	0(%r0),%r4
#	line 300, file "bitblt.c"
	movw	%r7,%r0
	subw2	&4,%r7
	LRSW3	0(%fp),%r4,%r1
	LLSW3	4(%fp),0(%r8),%r2
	orw2	%r2,%r1
	andw2	36(%fp),%r1
	xorw2	%r1,0(%r0)
#	line 301, file "bitblt.c"
	movw	16(%fp),%r3
	jnpos	.L156
.L159:
#	line 302, file "bitblt.c"
	movw	%r8,%r0
	subw2	&4,%r8
	LRSW3	0(%fp),0(%r0),%r0
	movw	%r0,%r4
#	line 303, file "bitblt.c"
	movw	%r7,%r0
	subw2	&4,%r7
	LLSW3	4(%fp),0(%r8),%r1
	orw2	%r4,%r1
	xorw2	%r1,0(%r0)
.L158:
#	line 304, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L159
.L157:
.L156:
#	line 305, file "bitblt.c"
	movw	0(%r8),%r4
#	line 306, file "bitblt.c"
	LRSW3	0(%fp),%r4,%r0
	LLSW3	4(%fp),-4(%r8),%r1
	orw2	%r1,%r0
	andw2	32(%fp),%r0
	xorw2	%r0,0(%r7)
 addw2	%r6,%r8
 addw2	%r5,%r7
.L154:
#	line 309, file "bitblt.c"
	subw2	&1,12(%fp)
	jpos	.L155
.L153:
	jmp	.L53
.L160:
#	line 312, file "bitblt.c"
	LRSW3	&2,16(%fp),%r0
	movw	%r0,4(%fp)
#	line 313, file "bitblt.c"
	andw3	&3,16(%fp),%r0
	movw	%r0,16(%fp)
#	line 314, file "bitblt.c"
	movw	12(%fp),%r4
.L163:
#	line 316, file "bitblt.c"
	movw	%r7,%r0
	addw2	&4,%r7
	movw	%r8,%r1
	addw2	&4,%r8
	andw3	0(%r1),32(%fp),%r1
	xorw2	%r1,0(%r0)
#	line 317, file "bitblt.c"
	movw	4(%fp),%r3
	jnpos	.L164
.L167:
#	line 318, file "bitblt.c"
	xorw2	0(%r8),0(%r7)
#	line 319, file "bitblt.c"
	xorw2	4(%r8),4(%r7)
#	line 320, file "bitblt.c"
	xorw2	8(%r8),8(%r7)
#	line 321, file "bitblt.c"
	xorw2	12(%r8),12(%r7)
#	line 322, file "bitblt.c"
	addw2	&16,%r7
#	line 323, file "bitblt.c"
	addw2	&16,%r8
.L166:
#	line 324, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L167
.L165:
.L164:
#	line 325, file "bitblt.c"
	movw	16(%fp),%r3
	jnpos	.L168
.L171:
#	line 326, file "bitblt.c"
	movw	%r7,%r0
	addw2	&4,%r7
	movw	%r8,%r1
	addw2	&4,%r8
	xorw2	0(%r1),0(%r0)
.L170:
#	line 327, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L171
.L169:
.L168:
#	line 328, file "bitblt.c"
	andw3	0(%r8),36(%fp),%r0
	xorw2	%r0,0(%r7)
 addw2	%r6,%r8
 addw2	%r5,%r7
.L162:
#	line 331, file "bitblt.c"
	subw2	&1,%r4
	jnz	.L163
.L161:
	jmp	.L53
.L172:
#	line 334, file "bitblt.c"
	cmpw	24(%fp),28(%fp)
	jle	.L173
#	line 335, file "bitblt.c"
	subw2	&4,%r8
.L173:
 PUSHW %ap
#	line 337, file "bitblt.c"
	LLSW3	&2,0(%fp),%r0
	movw	topbits(%r0),%r4
 MCOMW %r4, %r1
 MOVW %r4, %r2
 MOVW 0(%fp),%ap
 PUSHW %fp
BW_XORLOOP:
 ROTW %ap, 0(%r8), %r4
 ANDW2 %r2,%r4
 ADDW2 &4,%r8
 ROTW %ap,0(%r8),%r0
 ANDW3 %r0,%r1,%fp
 ORW2 %r4,%fp
 ANDW2 -0x28(%sp),%fp
 XORW2 %fp,0(%r7)
 ADDW2 &4,%r7
 MOVW -0x38(%sp),%r3
 BEB BW_XORINNER
.L176:
 ANDW3 %r2,%r0,%r4
 ADDW2 &4,%r8
 ROTW %ap, 0(%r8),%r0
 ANDW3 %r0,%r1,%fp
 ORW2 %r4,%fp
 XORW2 %fp,0(%r7)
 ADDW2 &4, %r7
.L175:
#	line 363, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L176
.L174:
BW_XORINNER:
 ROTW %ap, 0(%r8),%r4
 ANDW2 %r2,%r4
 LRSW3 %ap, 4(%r8),%r0
 ORW2 %r4,%r0
 ANDW2 -0x24(%sp),%r0
 XORW2 %r0,0(%r7)
 ADDW2	%r6,%r8
 ADDW2	%r5,%r7
 DECW -0x3c(%sp)
 BGB BW_XORLOOP
 POPW %fp
 POPW %ap
	jmp	.L53
.L177:
#	line 395, file "bitblt.c"
	LRSW3	&2,16(%fp),%r0
	movw	%r0,4(%fp)
#	line 396, file "bitblt.c"
	andw3	&3,16(%fp),%r0
	movw	%r0,16(%fp)
#	line 397, file "bitblt.c"
	movw	12(%fp),%r4
.L180:
#	line 399, file "bitblt.c"
	andw3	0(%r7),52(%fp),%r0
	movw	%r8,%r1
	subw2	&4,%r8
	andw3	0(%r1),36(%fp),%r1
	orw2	%r1,%r0
	movw	%r0,0(%r7)
#	line 400, file "bitblt.c"
	subw2	&4,%r7
#	line 401, file "bitblt.c"
	movw	4(%fp),%r3
	jnpos	.L181
.L184:
#	line 402, file "bitblt.c"
	movw	0(%r8),0(%r7)
#	line 403, file "bitblt.c"
	movw	-4(%r8),-4(%r7)
#	line 404, file "bitblt.c"
	movw	-8(%r8),-8(%r7)
#	line 405, file "bitblt.c"
	movw	-12(%r8),-12(%r7)
#	line 406, file "bitblt.c"
	subw2	&16,%r7
#	line 407, file "bitblt.c"
	subw2	&16,%r8
.L183:
#	line 408, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L184
.L182:
.L181:
#	line 409, file "bitblt.c"
	movw	16(%fp),%r3
	jnpos	.L185
.L188:
#	line 410, file "bitblt.c"
	movw	%r7,%r0
	subw2	&4,%r7
	movw	%r8,%r1
	subw2	&4,%r8
	movw	0(%r1),0(%r0)
.L187:
#	line 411, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L188
.L186:
.L185:
#	line 412, file "bitblt.c"
	andw3	0(%r7),48(%fp),%r0
	andw3	0(%r8),32(%fp),%r1
	orw2	%r1,%r0
	movw	%r0,0(%r7)
 addw2	%r6,%r8
 addw2	%r5,%r7
.L179:
#	line 415, file "bitblt.c"
	subw2	&1,%r4
	jnz	.L180
.L178:
	jmp	.L53
.L189:
#	line 418, file "bitblt.c"
	cmpw	24(%fp),28(%fp)
	jge	.L190
#	line 419, file "bitblt.c"
	addw2	&4,%r8
.L190:
.L193:
#	line 421, file "bitblt.c"
	movw	%r8,%r0
	subw2	&4,%r8
	movw	0(%r0),%r4
#	line 423, file "bitblt.c"
	LRSW3	0(%fp),%r4,%r0
	LLSW3	4(%fp),0(%r8),%r1
	orw2	%r1,%r0
	andw2	36(%fp),%r0
	andw3	52(%fp),0(%r7),%r1
	orw2	%r1,%r0
	movw	%r0,0(%r7)
#	line 424, file "bitblt.c"
	subw2	&4,%r7
#	line 425, file "bitblt.c"
	movw	16(%fp),%r3
	jnpos	.L194
.L197:
#	line 426, file "bitblt.c"
	movw	%r8,%r0
	subw2	&4,%r8
	LRSW3	0(%fp),0(%r0),%r0
	movw	%r0,%r4
#	line 427, file "bitblt.c"
	movw	%r7,%r0
	subw2	&4,%r7
	LLSW3	4(%fp),0(%r8),%r1
	orw2	%r4,%r1
	movw	%r1,0(%r0)
.L196:
#	line 428, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L197
.L195:
.L194:
#	line 429, file "bitblt.c"
	movw	0(%r8),%r4
#	line 431, file "bitblt.c"
	LRSW3	0(%fp),%r4,%r0
	LLSW3	4(%fp),-4(%r8),%r1
	orw2	%r1,%r0
	andw2	32(%fp),%r0
	andw3	48(%fp),0(%r7),%r1
	orw2	%r1,%r0
	movw	%r0,0(%r7)
 addw2	%r6,%r8
 addw2	%r5,%r7
.L192:
#	line 434, file "bitblt.c"
	subw2	&1,12(%fp)
	jpos	.L193
.L191:
	jmp	.L53
.L198:
#	line 437, file "bitblt.c"
	cmpw	16(%fp),&25
	jle	.L199
	jmp	.L200
.L199:
#	line 439, file "bitblt.c"
	subw3	16(%fp),&25,%r0
	movw	%r0,%r3
#	line 440, file "bitblt.c"
	LLSW3	&2,%r3,%r0
	LLSW3	&1,%r3,%r1
	addw2	%r1,%r0
	movw	%r0,%r4
 LLSW3 &0x2,0x10(%fp),%r0
 ADDW2 &4,%r0
 MOVAW B_FS_N,%r1
 ADDW2 %r4,%r1
#	line 446, file "bitblt.c"
	movw	12(%fp),%r4
 MOVW 0x20(%fp),%r2
 PUSHW %ap
 MOVW 0x24(%fp),%ap
 PUSHW %fp
 MOVW %r1,%fp
.L203:
 XORW3 0(%r7),0(%r8),%r1
 ANDW2 %r2,%r1
 XORW2 %r1,0(%r7)
 JMP     0(%fp)
B_FS_N: 
 MOVW    0x64(%r8),0x64(%r7)
 MOVW    0x60(%r8),0x60(%r7)
 MOVW    0x5c(%r8),0x5c(%r7)
 MOVW    0x58(%r8),0x58(%r7)
 MOVW    0x54(%r8),0x54(%r7)
 MOVW    0x50(%r8),0x50(%r7)
 MOVW    0x4c(%r8),0x4c(%r7)
 MOVW    0x48(%r8),0x48(%r7)
 MOVW    0x44(%r8),0x44(%r7)
 MOVW    0x40(%r8),0x40(%r7)
 MOVW    0x3c(%r8),0x3c(%r7)
 MOVW    0x38(%r8),0x38(%r7)
 MOVW    0x34(%r8),0x34(%r7)
 MOVW    0x30(%r8),0x30(%r7)
 MOVW    0x2c(%r8),0x2c(%r7)
 MOVW    0x28(%r8),0x28(%r7)
 MOVW    0x24(%r8),0x24(%r7)
 MOVW    0x20(%r8),0x20(%r7)
 MOVW    0x1c(%r8),0x1c(%r7)
 MOVW    0x18(%r8),0x18(%r7)
 MOVW    0x14(%r8),0x14(%r7)
 MOVW    0x10(%r8),0x10(%r7)
 MOVW    0xc(%r8),0xc(%r7)
 MOVW    0x8(%r8),0x8(%r7)
 MOVW    0x4(%r8),0x4(%r7)
 ADDW2   %r0,%r8
 ADDW2   %r0,%r7
 XORW3 0(%r7),0(%r8),%r1
 ANDW2 %ap,%r1
 XORW2 %r1,0(%r7)
 ADDW2   %r6,%r8
 ADDW2   %r5,%r7
.L202:
#	line 480, file "bitblt.c"
	subw2	&1,%r4
	jnz	.L203
.L201:
 POPW %fp
 POPW %ap
	jmp	.L53
.L200:
#	line 486, file "bitblt.c"
	LRSW3	&2,16(%fp),%r0
	movw	%r0,4(%fp)
#	line 487, file "bitblt.c"
	andw3	&3,16(%fp),%r0
	movw	%r0,16(%fp)
#	line 488, file "bitblt.c"
	movw	12(%fp),%r4
.L206:
#	line 490, file "bitblt.c"
	andw3	0(%r7),48(%fp),%r0
	movw	%r8,%r1
	addw2	&4,%r8
	andw3	0(%r1),32(%fp),%r1
	orw2	%r1,%r0
	movw	%r0,0(%r7)
#	line 491, file "bitblt.c"
	addw2	&4,%r7
#	line 492, file "bitblt.c"
	movw	4(%fp),%r3
	jnpos	.L207
.L210:
#	line 493, file "bitblt.c"
	movw	0(%r8),0(%r7)
#	line 494, file "bitblt.c"
	movw	4(%r8),4(%r7)
#	line 495, file "bitblt.c"
	movw	8(%r8),8(%r7)
#	line 496, file "bitblt.c"
	movw	12(%r8),12(%r7)
#	line 497, file "bitblt.c"
	addw2	&16,%r7
#	line 498, file "bitblt.c"
	addw2	&16,%r8
.L209:
#	line 499, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L210
.L208:
.L207:
#	line 500, file "bitblt.c"
	movw	16(%fp),%r3
	jnpos	.L211
.L214:
#	line 501, file "bitblt.c"
	movw	%r7,%r0
	addw2	&4,%r7
	movw	%r8,%r1
	addw2	&4,%r8
	movw	0(%r1),0(%r0)
.L213:
#	line 502, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L214
.L212:
.L211:
#	line 503, file "bitblt.c"
	andw3	0(%r7),52(%fp),%r0
	andw3	0(%r8),36(%fp),%r1
	orw2	%r1,%r0
	movw	%r0,0(%r7)
 addw2	%r6,%r8
 addw2	%r5,%r7
.L205:
#	line 506, file "bitblt.c"
	subw2	&1,%r4
	jnz	.L206
.L204:
	jmp	.L53
.L215:
#	line 509, file "bitblt.c"
	cmpw	24(%fp),28(%fp)
	jle	.L216
#	line 510, file "bitblt.c"
	subw2	&4,%r8
.L216:
.L219:
#	line 512, file "bitblt.c"
	movw	%r8,%r0
	addw2	&4,%r8
	movw	0(%r0),%r4
#	line 514, file "bitblt.c"
	LLSW3	4(%fp),%r4,%r0
	LRSW3	0(%fp),0(%r8),%r1
	orw2	%r1,%r0
	andw2	32(%fp),%r0
	andw3	48(%fp),0(%r7),%r1
	orw2	%r1,%r0
	movw	%r0,0(%r7)
#	line 515, file "bitblt.c"
	addw2	&4,%r7
#	line 516, file "bitblt.c"
	movw	16(%fp),%r3
	jnpos	.L220
.L223:
#	line 517, file "bitblt.c"
	movw	%r8,%r0
	addw2	&4,%r8
	LLSW3	4(%fp),0(%r0),%r0
	movw	%r0,%r4
#	line 518, file "bitblt.c"
	movw	%r7,%r0
	addw2	&4,%r7
	LRSW3	0(%fp),0(%r8),%r1
	orw2	%r4,%r1
	movw	%r1,0(%r0)
.L222:
#	line 519, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L223
.L221:
.L220:
#	line 520, file "bitblt.c"
	movw	0(%r8),%r4
#	line 522, file "bitblt.c"
	LLSW3	4(%fp),%r4,%r0
	LRSW3	0(%fp),4(%r8),%r1
	orw2	%r1,%r0
	andw2	36(%fp),%r0
	andw3	52(%fp),0(%r7),%r1
	orw2	%r1,%r0
	movw	%r0,0(%r7)
 addw2	%r6,%r8
 addw2	%r5,%r7
.L218:
#	line 525, file "bitblt.c"
	subw2	&1,12(%fp)
	jpos	.L219
.L217:
	jmp	.L53
.L54:
	cmpw	%r0,&0
	jl	.L224
	cmpw	%r0,&15
	jg	.L224
	ALSW3	&2,%r0,%r0
	jmp	*.L225(%r0)
	.data
	.align	4
#SWBEG
.L225:
	.word	.L215
	.word	.L88
	.word	.L130
	.word	.L172
	.word	.L198
	.word	.L76
	.word	.L118
	.word	.L160
	.word	.L189
	.word	.L67
	.word	.L109
	.word	.L151
	.word	.L177
	.word	.L55
	.word	.L97
	.word	.L139
#SWEND
	.text
.L224:
.L53:
	jmp	.L31
.L43:
#	line 537, file "bitblt.c"
	andh3	&31,16(%ap),%r0
	movw	%r0,%r4
#	line 538, file "bitblt.c"
	andh3	&31,4(%ap),%r0
	movw	%r0,%r6
#	line 539, file "bitblt.c"
	addw3	%r5,%r6,%r0
	cmpw	%r0,&31
	jle	.L226
#	line 541, file "bitblt.c"
	orw2	&4,20(%ap)
#	line 542, file "bitblt.c"
	LRSW3	%r6,&-1,%r0
	movw	%r0,32(%fp)
#	line 543, file "bitblt.c"
	addw3	%r5,%r6,%r0
	andw2	&31,%r0
	addw2	&1,%r0
	LLSW3	&2,%r0,%r0
	movw	topbits(%r0),36(%fp)
.L226:
#	line 546, file "bitblt.c"
	addw3	%r5,%r4,%r0
	cmpw	%r0,&31
	jleu	.L227
#	line 548, file "bitblt.c"
	orw2	&8,20(%ap)
#	line 549, file "bitblt.c"
	LRSW3	%r4,&-1,%r0
	movw	%r0,40(%fp)
#	line 550, file "bitblt.c"
	addw3	%r5,%r4,%r0
	andw2	&31,%r0
	addw2	&1,%r0
	LLSW3	&2,%r0,%r0
	movw	topbits(%r0),44(%fp)
.L227:
#	line 552, file "bitblt.c"
	movw	%r4,24(%fp)
#	line 553, file "bitblt.c"
	subw3	%r6,%r4,%r0
	movw	%r0,%r4
#	line 554, file "bitblt.c"
	movw	%r5,0(%fp)
#	line 556, file "bitblt.c"
	cmpw	0(%ap),12(%ap)
	jne	.L228
	cmph	6(%ap),18(%ap)
	jge	.L228
.L229:
#	line 558, file "bitblt.c"
	movtwh	%r3,%r0
	subh2	&1,%r0
	addh2	%r0,6(%ap)
#	line 559, file "bitblt.c"
	movtwh	%r3,%r0
	subh2	&1,%r0
	addh2	%r0,18(%ap)
#	line 560, file "bitblt.c"
	addw3	&4,0(%ap),%r0
	LLSW3	&2,0(%r0),%r0
	mnegw	%r0,%r0
	movw	%r0,%r6
#	line 561, file "bitblt.c"
	addw3	&4,12(%ap),%r0
	LLSW3	&2,0(%r0),%r0
	mnegw	%r0,%r0
	movw	%r0,%r5
	jmp	.L230
.L228:
#	line 565, file "bitblt.c"
	addw3	&4,0(%ap),%r0
	LLSW3	&2,0(%r0),%r0
	movw	%r0,%r6
#	line 566, file "bitblt.c"
	addw3	&4,12(%ap),%r0
	LLSW3	&2,0(%r0),%r0
	movw	%r0,%r5
.L230:
#	line 569, file "bitblt.c"
	pushw	0(%ap)
	pushw	4(%ap)
	call	&2,addr
	movw	%r0,%r8
#	line 570, file "bitblt.c"
	pushw	12(%ap)
	pushw	16(%ap)
	call	&2,addr
	movw	%r0,%r7
#	line 572, file "bitblt.c"
	movw	20(%ap),%r0
	jmp	.L232
.L233:
#	line 575, file "bitblt.c"
	addw3	&1,0(%fp),%r0
	LLSW3	&2,%r0,%r0
	LRSW3	24(%fp),topbits(%r0),%r0
	movw	%r0,32(%fp)
 MOVW 0x20(%fp),%r1
.L236:
 ROTW %r4,0(%r8),%r2
 XORW2 0(%r7),%r2
 ANDW2 %r1,%r2
 XORW2 %r2,0(%r7)
 ADDW2 %r6, %r8
 ADDW2 %r5, %r7
.L235:
#	line 584, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L236
.L234:
	jmp	.L231
.L237:
#	line 587, file "bitblt.c"
	subw3	%r4,&32,%r0
	movw	%r0,44(%fp)
#	line 588, file "bitblt.c"
	addw3	&1,0(%fp),%r0
	LLSW3	&2,%r0,%r0
	LRSW3	24(%fp),topbits(%r0),%r0
	movw	%r0,40(%fp)
 PUSHW %ap
 MOVW 0x28(%fp),%r0
 MOVW 0x2c(%fp),%ap
.L240:
 LLSW3 %ap,0(%r8),%r1
 LRSW3 %r4,4(%r8),%r2
 ORW2  %r2, %r1
 XORW2 0(%r7),%r1
 ANDW2 %r0,%r1
 XORW2 %r1,0(%r7)
 ADDW2 %r6, %r8
 ADDW2 %r5, %r7
.L239:
#	line 602, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L240
.L238:
 POPW %ap
	jmp	.L231
.L241:
 PUSHW %ap
 MOVW 0x28(%fp),%r0
 MOVW 0x2c(%fp),%ap
.L244:
 ROTW %r4,0(%r8),%r1
 XORW3 0(%r7),%r1,%r2
 ANDW2 %r0,%r2
 XORW2 %r2,0(%r7)
 XORW2 4(%r7),%r1
 ANDW2 %ap,%r1
 XORW2 %r1,4(%r7)
 ADDW2 %r6, %r8
 ADDW2 %r5, %r7
.L243:
#	line 619, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L244
.L242:
 POPW %ap
	jmp	.L231
.L245:
 PUSHW %ap
 SUBW3 %r4,&0x20,%ap
 MOVW 0x20(%fp),%r0
 MOVW 0x24(%fp),%r2
 PUSHW %fp
.L248:
 ANDW3 %r0, 0(%r8), %r1
 ANDW3 %r2, 4(%r8),%ap
 ORW2 %ap, %r1
 ROTW %r4, %r1, %r1
 XORW3 0(%r7), %r1, %ap
 ANDW2 0x28(%fp), %ap
 XORW2 %ap, 0(%r7)
 XORW2 4(%r7), %r1
 ANDW2 0x2c(%fp), %r1
 XORW2 %r1, 4(%r7)
 ADDW2 %r6, %r8
 ADDW2 %r5, %r7
.L247:
#	line 641, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L248
.L246:
 POPW %fp
 POPW %ap
	jmp	.L231
.L249:
#	line 646, file "bitblt.c"
	addw3	&1,0(%fp),%r0
	LLSW3	&2,%r0,%r0
	LRSW3	24(%fp),topbits(%r0),%r0
	movw	%r0,32(%fp)
 MOVW 0x20(%fp),%r1
.L252:
 ROTW %r4,0(%r8),%r2
 ANDW2 %r1,%r2
 ORW2 %r2,0(%r7)
 ADDW2 %r6, %r8
 ADDW2 %r5, %r7
.L251:
#	line 654, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L252
.L250:
	jmp	.L231
.L253:
 MOVW 0x20(%fp),%r0
 PUSHW %ap
 MOVW 0x24(%fp),%ap
.L256:
 ANDW3 %r0,0(%r8),%r2
 ANDW3 %ap,4(%r8),%r1
 ORW2 %r2,%r1
 ROTW %r4,%r1,%r1
 ORW2 %r1,0(%r7)
 ADDW2 %r6, %r8
 ADDW2 %r5, %r7
.L255:
#	line 669, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L256
.L254:
 POPW %ap
	jmp	.L231
.L257:
#	line 673, file "bitblt.c"
	cmpw	0(%fp),&16
	jg	.L258
 MOVW &0xffff0000,%r2
 ORW3 0x28(%fp),0x2c(%fp),%r0
.L261:
 ROTW %r4,0(%r8),%r1
 ANDW2 %r0,%r1
 ORH2 %r1,2(%r7)
 ANDW2 %r2, %r1
 ORW2 %r1, 4(%r7)
 ADDW2 %r6, %r8
 ADDW2 %r5, %r7
.L260:
#	line 685, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L261
.L259:
	jmp	.L262
.L258:
 MOVW 0x28(%fp),%r0
 PUSHW %ap
 MOVW 0x2c(%fp),%ap
.L265:
 ROTW %r4,0(%r8),%r1
 ANDW3 %r0,%r1,%r2
 ORW2 %r2,0(%r7)
 ANDW2 %ap,%r1
 ORW2 %r1,4(%r7)
 ADDW2 %r6, %r8
 ADDW2 %r5, %r7
.L264:
#	line 700, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L265
.L263:
 POPW %ap
.L262:
	jmp	.L231
.L266:
 MOVW 0x20(%fp),%r0
 PUSHW %ap
 MOVW 0x24(%fp),%ap
#	line 708, file "bitblt.c"
	cmpw	0(%fp),&16
	jle	.L267
.L270:
 ANDW3 %r0,0(%r8),%r1
 ANDW3 %ap,4(%r8),%r2
 ORW2 %r2,%r1
 ROTW %r4,%r1,%r1
 ANDW3 0x28(%fp),%r1,%r2
 ORW2 %r2,0(%r7)
 ANDW2 0x2c(%fp),%r1
 ORW2 %r1,4(%r7)
 ADDW2 %r6, %r8
 ADDW2 %r5, %r7
.L269:
#	line 720, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L270
.L268:
	jmp	.L271
.L267:
.L274:
 ANDW3 %r0,0(%r8),%r1
 ANDW3 %ap,4(%r8),%r2
 ORW2 %r2,%r1
 ROTW %r4,%r1,%r1
 ORH2 %r1,2(%r7)
 ANDW2 &0xffff0000,%r1
 ORW2 %r1,4(%r7)
 ADDW2 %r6, %r8
 ADDW2 %r5, %r7
.L273:
#	line 734, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L274
.L272:
.L271:
 POPW %ap
	jmp	.L231
.L275:
#	line 739, file "bitblt.c"
	addw3	&1,0(%fp),%r0
	LLSW3	&2,%r0,%r0
	LRSW3	24(%fp),topbits(%r0),%r0
	movw	%r0,32(%fp)
 MOVW 0x20(%fp),%r1
.L278:
 ROTW %r4,0(%r8),%r2
 ANDW2 %r1,%r2
 MCOMW %r2,%r2
 ANDW2 %r2,0(%r7)
 ADDW2 %r6, %r8
 ADDW2 %r5, %r7
.L277:
#	line 748, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L278
.L276:
	jmp	.L231
.L279:
 MOVW 0x20(%fp),%r0
 PUSHW %ap
 MOVW 0x24(%fp),%ap
.L282:
 ANDW3 %r0,0(%r8),%r2
 ANDW3 %ap,4(%r8),%r1
 ORW2 %r2,%r1
 ROTW %r4,%r1,%r1
 MCOMW %r1,%r1
 ANDW2 %r1,0(%r7)
 ADDW2 %r6, %r8
 ADDW2 %r5, %r7
.L281:
#	line 764, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L282
.L280:
 POPW %ap
	jmp	.L231
.L283:
#	line 768, file "bitblt.c"
	cmpw	0(%fp),&16
	jg	.L284
 MOVW &0xffff,%r2
 ORW3 0x28(%fp),0x2c(%fp),%r0
.L287:
 ROTW %r4,0(%r8),%r1
 ANDW2 %r0,%r1
 MCOMW %r1,%r1
 ANDH2 %r1,2(%r7)
 ORW2 %r2,%r1
 ANDW2 %r1,4(%r7)
 ADDW2 %r6, %r8
 ADDW2 %r5, %r7
.L286:
#	line 781, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L287
.L285:
	jmp	.L288
.L284:
 MOVW 0x28(%fp),%r0
 PUSHW %ap
 MOVW 0x2c(%fp),%ap
.L291:
 ROTW %r4,0(%r8),%r1
 ANDW3 %r0,%r1,%r2
 MCOMW %r2,%r2
 ANDW2 %r2,0(%r7)
 ANDW2 %ap,%r1
 MCOMW %r1,%r1
 ANDW2 %r1,4(%r7)
 ADDW2 %r6, %r8
 ADDW2 %r5, %r7
.L290:
#	line 798, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L291
.L289:
 POPW %ap
.L288:
	jmp	.L231
.L292:
 MOVW 0x20(%fp),%r0
 PUSHW %ap
 MOVW 0x24(%fp),%ap
#	line 806, file "bitblt.c"
	cmpw	0(%fp),&16
	jle	.L293
.L296:
 ANDW3 %r0,0(%r8),%r1
 ANDW3 %ap,4(%r8),%r2
 ORW2 %r2,%r1
 ROTW %r4,%r1,%r1
 ANDW3 0x28(%fp),%r1,%r2
 MCOMW %r2,%r2
 ANDW2 %r2,0(%r7)
 ANDW2 0x2c(%fp),%r1
 MCOMW %r1,%r1
 ANDW2 %r1,4(%r7)
 ADDW2 %r6, %r8
 ADDW2 %r5, %r7
.L295:
#	line 820, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L296
.L294:
	jmp	.L297
.L293:
.L300:
 ANDW3 %r0,0(%r8),%r1
 ANDW3 %ap,4(%r8),%r2
 ORW2 %r2,%r1
 ROTW %r4,%r1,%r1
 MCOMW %r1,%r1
 ANDH2 %r1,2(%r7)
 ORW2 &0xffff,%r1
 ANDW2 %r1,4(%r7)
 ADDW2 %r6, %r8
 ADDW2 %r5, %r7
.L299:
#	line 835, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L300
.L298:
.L297:
 POPW %ap
	jmp	.L231
.L301:
#	line 840, file "bitblt.c"
	addw3	&1,0(%fp),%r0
	LLSW3	&2,%r0,%r0
	LRSW3	24(%fp),topbits(%r0),%r0
	movw	%r0,32(%fp)
 MOVW 0x20(%fp),%r1
.L304:
 ROTW %r4,0(%r8),%r2
 ANDW2 %r1,%r2
 XORW2 %r2,0(%r7)
 ADDW2 %r6, %r8
 ADDW2 %r5, %r7
.L303:
#	line 848, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L304
.L302:
	jmp	.L231
.L305:
 MOVW 0x20(%fp),%r0
 PUSHW %ap
 MOVW 0x24(%fp),%ap
.L308:
 ANDW3 %r0,0(%r8),%r2
 ANDW3 %ap,4(%r8),%r1
 ORW2 %r2,%r1
 ROTW %r4,%r1,%r1
 XORW2 %r1,0(%r7)
 ADDW2 %r6, %r8
 ADDW2 %r5, %r7
.L307:
#	line 863, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L308
.L306:
 POPW %ap
	jmp	.L231
.L309:
#	line 867, file "bitblt.c"
	cmpw	0(%fp),&16
	jg	.L310
 MOVW &0xffff0000,%r2
 ORW3 0x28(%fp),0x2c(%fp),%r0
.L313:
 ROTW %r4,0(%r8),%r1
 ANDW2 %r0,%r1
 XORH2 %r1,2(%r7)
 ANDW2 %r2,%r1
 XORW2 %r1,4(%r7)
 ADDW2 %r6, %r8
 ADDW2 %r5, %r7
.L312:
#	line 879, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L313
.L311:
	jmp	.L314
.L310:
 MOVW 0x28(%fp),%r0
 PUSHW %ap
 MOVW 0x2c(%fp),%ap
.L317:
 ROTW %r4,0(%r8),%r1
 ANDW3 %r0,%r1,%r2
 XORW2 %r2,0(%r7)
 ANDW2 %ap,%r1
 XORW2 %r1,4(%r7)
 ADDW2 %r6, %r8
 ADDW2 %r5, %r7
.L316:
#	line 894, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L317
.L315:
 POPW %ap
.L314:
	jmp	.L231
.L318:
 MOVW 0x20(%fp),%r0
 PUSHW %ap
 MOVW 0x24(%fp),%ap
#	line 902, file "bitblt.c"
	cmpw	0(%fp),&16
	jle	.L319
.L322:
 ANDW3 %r0,0(%r8),%r1
 ANDW3 %ap,4(%r8),%r2
 ORW2 %r2,%r1
 ROTW %r4,%r1,%r1
 ANDW3 0x28(%fp),%r1,%r2
 XORW2 %r2,0(%r7)
 ANDW2 0x2c(%fp),%r1
 XORW2 %r1,4(%r7)
 ADDW2 %r6, %r8
 ADDW2 %r5, %r7
.L321:
#	line 914, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L322
.L320:
	jmp	.L323
.L319:
.L326:
 ANDW3 %r0,0(%r8),%r1
 ANDW3 %ap,4(%r8),%r2
 ORW2 %r2,%r1
 ROTW %r4,%r1,%r1
 XORH2 %r1,2(%r7)
 ANDW2 &0xffff0000,%r1
 XORW2 %r1,4(%r7)
 ADDW2 %r6, %r8
 ADDW2 %r5, %r7
.L325:
#	line 928, file "bitblt.c"
	subw2	&1,%r3
	jpos	.L326
.L324:
.L323:
 POPW %ap
	jmp	.L231
.L232:
	cmpw	%r0,&0
	jl	.L327
	cmpw	%r0,&15
	jg	.L327
	ALSW3	&2,%r0,%r0
	jmp	*.L328(%r0)
	.data
	.align	4
#SWBEG
.L328:
	.word	.L233
	.word	.L249
	.word	.L275
	.word	.L301
	.word	.L237
	.word	.L253
	.word	.L279
	.word	.L305
	.word	.L241
	.word	.L257
	.word	.L283
	.word	.L309
	.word	.L245
	.word	.L266
	.word	.L292
	.word	.L318
#SWEND
	.text
.L327:
.L231:
	jmp	.L31
.L31:
	.def	.ef;	.val	.;	.scl	101;	.line	909;	.endef
	.ln	909
	.set	.F1,64
	.set	.R1,6
	ret	&.R1
	.def	bitblt;	.val	.;	.scl	-1;	.endef
	.data
