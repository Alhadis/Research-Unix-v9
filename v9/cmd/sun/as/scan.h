/*	@(#)scan.h 1.1 86/02/03 SMI */

#include <stab.h>

/* stab symbol table bucket  */
struct stab_sym_bkt {
         char                *ch;        /* string name   */
         unsigned char       type;       /* field "type"  */
         char                other;      /* field "other" */
         short               desc;       /* field "desc"  */
         unsigned long       value;      /* field "value" */
         short               id;         /* character count of string */
         struct sym_bkt	     *label;	 /* pointer to the label symbol */
         struct stab_sym_bkt *next_stab;
};

struct stab_sym_bkt *stabkt_head,   /* head of stab symbol bucket linked list */
                    *stabkt_tail;   /* tail of stab symbol bkcket linked list */

