#ifndef lint
static  char sccsid[] = "@(#)genassym.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "../h/param.h"
#include "../h/buf.h"
#include "../h/vmmeter.h"
#include "../h/vmparam.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/cmap.h"
#include "../h/map.h"
#include "../h/proc.h"
#include "../h/msgbuf.h"
#include "../h/vmmac.h"

#include "../machine/pte.h"
#include "../machine/reg.h"
#include "../machine/psl.h"
#include "../machine/mmu.h"
#include "../machine/cpu.h"
#include "../machine/scb.h"
#include "../machine/clock.h"
#include "../machine/memerr.h"
#include "../machine/interreg.h"
#include "../machine/eeprom.h"
#include "../machine/eccreg.h"

#include "../machine/zscom.h"

struct zsops *zs_proto[] = { 0 };


main()
{
	register struct proc *p = (struct proc *)0;
	register struct vmmeter *vm = (struct vmmeter *)0;
	register struct user *up = (struct user *)0;
	register struct regs *rp = (struct regs *)0;
	register struct context *ctx = (struct context *)0;
	register struct zscom *zs = (struct zscom *)0;

	printf("#define\tP_LINK 0x%x\n", &p->p_link);
	printf("#define\tP_RLINK 0x%x\n", &p->p_rlink);
	printf("#define\tP_ADDR 0x%x\n", &p->p_addr);
	printf("#define\tP_PRI 0x%x\n", &p->p_pri);
	printf("#define\tP_STAT 0x%x\n", &p->p_stat);
	printf("#define\tP_WCHAN 0x%x\n", &p->p_wchan);
	printf("#define\tP_CTX 0x%x\n", &p->p_ctx);
	printf("#define\tP_FLAG 0x%x\n", &p->p_flag);
	printf("#define\tP_PID 0x%x\n", &p->p_pid);
	printf("#define\tPROCSIZE 0x%x\n", sizeof (struct proc));
	printf("#define\tSLOAD 0x%x\n", SLOAD);
	printf("#define\tSSLEEP 0x%x\n", SSLEEP);
	printf("#define\tSRUN 0x%x\n", SRUN);
	printf("#define\tSPTECHG_BIT %d\n", bit(SPTECHG));
	printf("#define\tSPROCIO_BIT %d\n", bit(SPROCIO));
	printf("#define\tV_SWTCH 0x%x\n", &vm->v_swtch);
	printf("#define\tV_TRAP 0x%x\n", &vm->v_trap);
	printf("#define\tV_SYSCALL 0x%x\n", &vm->v_syscall);
	printf("#define\tV_INTR 0x%x\n", &vm->v_intr);
	printf("#define\tV_PDMA 0x%x\n", &vm->v_pdma);
	printf("#define\tMSGBUFPTECNT 0x%x\n", btoc(sizeof (struct msgbuf)));
/*
	printf("#define\tNMBCLUSTERS 0x%x\n", NMBCLUSTERS);
*/
	printf("#define\tR_SP 0x%x\n", &rp->r_sp);
	printf("#define\tR_PC 0x%x\n", &rp->r_pc);
	printf("#define\tR_SR 0x%x\n", ((int)&rp->r_sr) + sizeof (short));
	printf("#define\tR_VOR 0x%x\n", ((int)&rp->r_pc) + sizeof (int));
	printf("#define\tPCB_REGS 0x%x\n", up->u_pcb.pcb_regs);
	printf("#define\tPCB_SR 0x%x\n", &up->u_pcb.pcb_sr);
	printf("#define\tPCB_SSWAP 0x%x\n", &up->u_pcb.pcb_sswap);
	printf("#define\tPCB_P0LR 0x%x\n", &up->u_pcb.pcb_p0lr);
	printf("#define\tAST_SCHED_BIT %d\n", bit(AST_SCHED));
	printf("#define\tAST_STEP_BIT %d\n", bit(AST_STEP));
	printf("#define\tTRACE_USER_BIT %d\n", bit(TRACE_USER));
	printf("#define\tTRACE_AST_BIT %d\n", bit(TRACE_AST));
	printf("#define\tSR_SMODE_BIT %d\n", bit(SR_SMODE));
	printf("#define\tSR_TRACE_BIT %d\n", bit(SR_TRACE));
	printf("#define\tIR_SOFT_INT1_BIT %d\n", bit(IR_SOFT_INT1));
	printf("#define\tIR_SOFT_INT2_BIT %d\n", bit(IR_SOFT_INT2));
	printf("#define\tIR_SOFT_INT3_BIT %d\n", bit(IR_SOFT_INT3));
	printf("#define\tU_STACK 0x%x\n", up->u_stack);
	printf("#define\tU_LOFAULT 0x%x\n", &up->u_lofault);
	printf("#define\tU_FP_ISTATE 0x%x\n", &up->u_fp_istate);
	printf("#define\tU_FPS_REGS 0x%x\n", up->u_fp_status.fps_regs);
	printf("#define\tU_FPS_CTRL 0x%x\n", &up->u_fp_status.fps_control);
	printf("#define\tUSIZE 0x%x\n", sizeof (struct user));
	printf("#define\tZSSIZE 0x%x\n", sizeof (struct zscom));
	printf("#define\tZS_ADDR 0x%x\n", &zs->zs_addr);
	printf("#define\tCTX_CONTEXT 0x%x\n", &ctx->ctx_context);
	printf("#define\tCTX_PROCP 0x%x\n", &ctx->ctx_procp);
	printf("#define\tU_MAPVAL 0x%x\n", ((int)&u & PAGEADDRBITS) | PAGEBASE);
	printf("#define\tEEPROM_ADDR_MAPVAL 0x%x\n",
	    ((int)EEPROM_ADDR & PAGEADDRBITS) | PAGEBASE);
	printf("#define\tEEPROM_ADDR_PTE 0x%x\n",
	    PG_V | PG_KW | PGT_OBIO | btop(OBIO_EEPROM_ADDR));
	printf("#define\tCLKADDR_MAPVAL 0x%x\n",
	    ((int)CLKADDR & PAGEADDRBITS) | PAGEBASE);
	printf("#define\tCLKADDR_PTE 0x%x\n",
	    PG_V | PG_KW | PGT_OBIO | btop(OBIO_CLKADDR));
	printf("#define\tMEMREG_MAPVAL 0x%x\n",
	    ((int)MEMREG & PAGEADDRBITS) | PAGEBASE);
	printf("#define\tMEMREG_PTE 0x%x\n",
	    PG_V | PG_KW | PGT_OBIO | btop(OBIO_MEMREG));
	printf("#define\tINTERREG_MAPVAL 0x%x\n",
	    ((int)INTERREG & PAGEADDRBITS) | PAGEBASE);
	printf("#define\tINTERREG_PTE 0x%x\n",
	    PG_V | PG_KW | PGT_OBIO | btop(OBIO_INTERREG));
	printf("#define\tECCREG_MAPVAL 0x%x\n",
	    ((int)ECCREG & PAGEADDRBITS) | PAGEBASE);
	printf("#define\tECCREG_PTE 0x%x\n",
	    PG_V | PG_KW | PGT_OBIO | btop(OBIO_ECCREG));
	printf("#define\tSCBSIZE 0x%x\n", sizeof (struct scb));
	printf("#define\tROMP_PRINTF 0x%x\n", &romp->v_printf);
	printf("#define\tROMP_ROMVEC_VERSION 0x%x\n", &romp->v_romvec_version);
	printf("#define\tROMP_MEMORYBITMAP 0x%x\n", &romp->v_memorybitmap);
	exit(0);
}

bit(mask)
	register long mask;
{
	register int i;

	for (i = 0; i < 32; i++) {
		if (mask & 1)
			return (i);
		mask >>= 1;
	}
	exit (1);
}
