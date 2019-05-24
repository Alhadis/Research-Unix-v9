/*
 * init the symbol table
 * symbol values in vax order??
 */

#include "defs.h"
#include "sym.h"
#include "a.out.h"
#include <sys/types.h>

#define	NSYMS	300
#define	BIGBUF	(4*4096)

extern struct sym *symtab;

syminit(h)
struct exec *h;
{
	register struct nlist *q;
	register n, m;
	struct nlist space[BIGBUF/sizeof(struct nlist)];
	struct sym *sbuf;
	register struct sym *cur;
	struct sym *curgbl;
	register int type, ltype;
	off_t off;
	char *malloc();
	char *savename();

	symtab = NULL;
	if (h->a_syms == 0)
		return;		/* stripped */
	off = h->a_text + h->a_data;
	if ((h->a_flag & 01) == 0)
		off += off;	/* space for reloc data */
	lseek(fsym, off + sizeof(*h), 0);
	curgbl = sbuf = cur = NULL;
	for (n = h->a_syms; n > 0 ;) {
		m = read(fsym, (char *)space, min(n, sizeof(space)));
		if (m <= 0)
			break;
		n -= m;
		for (q = space; m > 0; q++, m-= sizeof(space[0])) {
			switch (q->n_type) {
			case N_ABS|N_EXT:
				type = S_ABS;
				break;

			case N_TEXT:
			case N_TEXT|N_EXT:
				type = S_TEXT;
				break;

			case N_DATA:
			case N_BSS:	/* ?? file statics ?? */
			case N_DATA|N_EXT:
			case N_BSS|N_EXT:
				type = S_DATA;
				break;

			case N_ABS:
				ltype = S_LSYM;
				type = S_STAB;
				break;

			default:
				continue;
			}
			if (sbuf == NULL || ++cur >= sbuf + NSYMS) {
				if ((sbuf = (struct sym *)malloc(sizeof(struct sym) * NSYMS)) == NULL) {
					printf("out of mem for syms\n");
					return;
				}
				cur = sbuf;
			}
			cur->y_type = type;
			cur->y_ltype = ltype;
			cur->y_value = q->n_value;
			cur->y_name = savename(q->n_name, sizeof(q->n_name));
			cur->y_locals = NULL;
			if (cur->y_type == S_TEXT && q->n_name[0] == '~') {
				cur->y_next = curgbl;
				curgbl = cur;
			}
			else if (cur->y_type == S_STAB) {
				if (curgbl == NULL)
					continue;
				cur->y_next = curgbl->y_locals;
				curgbl->y_locals = cur;
			}
			else {
				cur->y_next = symtab;
				symtab = cur;
			}
		}
	}
	for (; curgbl; curgbl = curgbl->y_next) {
		if (curgbl->y_locals == NULL)
			continue;
		for (cur = symtab; cur; cur = cur->y_next) {
			if (cur->y_type != S_TEXT)
				continue;
			if (cur->y_value == curgbl->y_value) {
				cur->y_locals = curgbl->y_locals;
				break;
			}
		}
	}
}

static char *
savename(n, sz)
char *n;
register int sz;
{
	char *p;
	char *malloc();

	if ((p = malloc(sz+1)) == NULL)
		return ("");
	strncpy(p, n, sz);
	p[sz] = 0;
	return (p);
}

eqsym(sp, n)
struct sym *sp;
char *n;
{

	return (strcmp(sp->y_name, n) == 0);
}
