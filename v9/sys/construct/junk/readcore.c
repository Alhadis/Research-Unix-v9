#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <machine/reg.h>

char xx[1024 * 8];
struct user *u = (struct user *)xx;

main()
{
	FILE *fp;
	int *r;

	if ((fp = fopen("core", "r")) == NULL) {
		printf("Can't open core file\n");
		exit(1);
	}

	fread(u, sizeof(xx), 1, fp);

	printf("u.u_ar0 = 0x%x\n", u->u_ar0);

	r = (int *)&xx[((int)u->u_ar0) & 0x1fff];
	printf("D0/%x	A0/%x\n", r[R0], r[AR0]);
	printf("D1/%x	A1/%x\n", r[R1], r[AR1]);
	printf("D2/%x	A2/%x\n", r[R2], r[AR2]);
	printf("D3/%x	A3/%x\n", r[R3], r[AR3]);
	printf("D4/%x	A4/%x\n", r[R4], r[AR4]);
	printf("D5/%x	A5/%x\n", r[R5], r[AR5]);
	printf("D6/%x	A6/%x\n", r[R6], r[AR6]);
	printf("D7/%x	SP/%x\n", r[R7], r[SP]);
	printf("PS/%x	PC/%x\n", r[PS], r[PC]);

	fclose(fp);
}
