#ifndef lint
static	char sccsid[] = "@(#)rel.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "as.h"
#include <a.out.h>

struct reloc{ struct relocation_info r; struct sym_bkt *rsymbol; };

/*  Handle output file processing for b.out files */

FILE *tout;		/* text portion of output file */
FILE *dout;		/* data portion of output file */
FILE *d1out;		/* data portion of output file */
FILE *d2out;		/* data portion of output file */
FILE *rtout;		/* text relocation commands */
FILE *rdout;		/* data relocation commands */

long rtsize;		/* size of text relocation area */
long rdsize;		/* size of data relocation area */

char *rname = "/tmp/as68XXXXXXXX";	/* name of file for relocation commands */

extern int ext_instruction_set;		/* = 1 for the 68020 assembler */

struct exec filhdr;	/* header for a.out files, contains sizes */

/* Initialize files for output and write out the header */
rel_header()
{ 
	if ((tout = fopen(rel_name, "w")) == NULL ||
	    (dout = fopen(rel_name, "a")) == NULL ||
	    (d1out = fopen(rel_name, "a")) == NULL ||
	    (d2out = fopen(rel_name, "a")) == NULL)
	   sys_error("open on output file %s failed", rel_name);

	/* Concat(rname, Source_name, ".tmpr"); */
	mktemp( rname );
	if ((rtout = fopen(rname, "w")) == NULL
	   || (rdout = fopen(rname, "a")) == NULL)
	   sys_error("open on output file %s failed", rname);
	filhdr.a_machtype = (ext_instruction_set ? M_68020 : M_68010);
	filhdr.a_magic = OMAGIC;
	filhdr.a_text = tsize;
	filhdr.a_data = dsize+d1size+d2size;
	filhdr.a_trsize = rtsize;
	filhdr.a_drsize = rdsize;
	filhdr.a_bss = bsize;
	filhdr.a_entry = 0;

	fseek(tout, 0L, 0);
	fwrite(&filhdr, sizeof(filhdr), 1, tout);

	fseek(tout, (long)(N_TXTOFF(filhdr)), 0);   /* seek to start of text */
	fseek(dout, (long)(N_TXTOFF(filhdr)+tsize), 0);	
	fseek(rdout, rtsize, 0);
	fseek(d1out, (long)(N_TXTOFF(filhdr)+tsize+dsize), 0);
	fseek(d2out, (long)(N_TXTOFF(filhdr)+tsize+dsize+d1size), 0);
	rtsize = 0;
	rdsize = 0;

} /* end Rel_Header * /

/*
 * Fix_Rel -	Fix up the object file
 *	For .b files, we have to
 *	1)	append the relocation segments
 *	2)	fix up the rtsize and rdsize in the header
 *	3)	delete the temporary file for relocation commands
 */
fix_rel()
{
	register FILE *fin, *fout;
	long ortsize;
	long i;
#	define FUDGE( x ) ((x)/sizeof(struct reloc))*sizeof(struct relocation_info)

	ortsize = filhdr.a_trsize;
	if (ortsize < rtsize ) sys_error("First pass relocation botch\n");
	filhdr.a_trsize = FUDGE(rtsize);
	if( rflag ){
	    filhdr.a_trsize += FUDGE(rdsize);
	    filhdr.a_drsize  = 0;
	    filhdr.a_text   += filhdr.a_data;
	    filhdr.a_data    = 0;
	} else
	    filhdr.a_drsize = FUDGE(rdsize);
	fclose(rtout);
	fclose(rdout);
	if ((fin = fopen(rname, "r")) == NULL)
	   sys_error("cannot reopen relocation file %s", rname);

	fout = tout;

	/* first write text relocation commands */
	fseek(fout, (long)(N_TXTOFF(filhdr)+filhdr.a_text+filhdr.a_data), 0);
	redosyms();
	dorseg(fin,fout,rtsize);

	/* seek to start of data segment relocation commands */
	fseek(fin, ortsize, 0);
	dorseg(fin,fout,rdsize);
	/* Now put the full symbol table out there */
	filhdr.a_syms = sym_write(fout);

	/* Now for the string table */
	i = str_write(fout);
	/* After the table is written, rewrite the length */
	fseek(fout,N_STROFF(filhdr),0);
	fwrite(&i,sizeof(long),1,fout);

	/* now re-write header */
	fseek(fout, 0, 0);
	fwrite(&filhdr, sizeof(filhdr), 1, fout);
	fclose(fin);
	unlink(rname);
#	undef FUDGE
} /* Fix_Rel */


dorseg(fin,fout,size)
	register size;
	FILE *fin,*fout;
{
	struct reloc new;
	while(size) {
		size -= sizeof(new);
		fread(&new,sizeof(new),1,fin);
		if (new.r.r_extern == 1){
		   new.r.r_symbolnum = new.rsymbol->final;
		} 
		fwrite(&new.r ,sizeof(new.r ),1,fout);
        } /* end while */
} /* end dorseg */

/* rel_val -	Puts value of operand into next bytes of Code
 * updating Code_length. Put_Rel is called to handle possible relocation.
 * If size=L a longword is stored, otherwise a word is stored 
 */
rel_val(opnd,size, pcrel)
	register struct oper *opnd;
	subop_t size;
{
	register int i;
	register struct sym_bkt *sp;
	long val;
	char *ccode;			/* char version of this */
	union 				/* floating point equivalence */
		{ 
		unsigned short words[6] ;
		float f ;
		double d ;
		/* extended x ; bcdrecord p ; */
		}
		float_equivalence ;

	i = code_length>>1;	/* get index into WCode */
	if (sp = opnd->sym_o)
	   put_rel(opnd, size, dot + code_length, pcrel);
	val = opnd->value_o;
/*	/* 
/*	 * if we're doing pc-relative relocation, we must bias this data
/*	 * by the negative of its own address. The linker, ld, will add in
/*	 * the absolute address, then subtract out this assembly's relocation
/*	 * constant, leaving us with the pc-relative value.
/*	 */
/*	if (pcrel)
/*		val -= dot + code_length;
*/
	switch(size) {
	  case SUBOP_D:		/* Op code specifies double operand. */
	  	switch(opnd->immed_o)
			{
		case T_FLOAT : 	/* Immediate operand was actually single. */
			float_equivalence.d = (double) opnd->fval_o ;
			break ;
		case T_DOUBLE :	/* Immediate operand was actually double. */
			float_equivalence.d = (double) opnd->dval_o ;
			break ;
		default:	/* Immediate operand was integer. */
			float_equivalence.d = (double) opnd->value_o ;
			break ;
			}
	  	wcode[i++] = float_equivalence.words[0] ;
	  	wcode[i++] = float_equivalence.words[1] ;
	  	wcode[i++] = float_equivalence.words[2] ;
	  	wcode[i++] = float_equivalence.words[3] ;
		code_length += 8 ;
		break ;

	  case SUBOP_S:		/* Op code specifies single operand. */
	  	switch(opnd->immed_o)
			{
		case T_FLOAT :	/* Immediate operand was actually single. */
			float_equivalence.f = (float) opnd->fval_o ;
			break ;
		case T_DOUBLE :	/* Immediate operand was actually double. */
			float_equivalence.f = (float) opnd->dval_o ;
			break ;
		default:	/* Immediate operand was integer. */
			float_equivalence.f = (float) opnd->value_o ;
			break ;
			}
	  	wcode[i++] = float_equivalence.words[0] ;
	  	wcode[i++] = float_equivalence.words[1] ;
		code_length += 4 ;
		break ;

	  case SUBOP_L: wcode[i++] = val>>16;
		  code_length += 2;
		  val &= 0xFFFF;
		  /* fall through ... */
	  case SUBOP_W: 
	  default:
		  if ((short)val > 32767 || (short)val < -32768)
			PROG_ERROR( E_OFFSET );
		  wcode[i] = val;
		  code_length += 2;
		  break;
	  case SUBOP_B: 
		  if ((char)val > 255 || (char)val < -256)
			PROG_ERROR( E_OFFSET );
	  	  ccode = (char *)wcode;
		  ccode[code_length++] = val;
	}
} /* end rel_val */

/* Version of Put_Text which puts whole words, thus enforcing the mapping
 * of bytes to words.
 */

#ifdef mc68000
put_words(code,nbytes)
	char *code;
{	
	if (nbytes & 1) sys_error("Put_Words given odd nbytes=%d\n",nbytes);
	put_text(code,nbytes);
}
#endif

#ifndef mc68000
Put_Words(code,nbytes)
register char *code;
{	register char *cc, ch;
	register int i;
	char tcode[100];

	cc = tcode;
	for (i=0; i<nbytes; i++) tcode[i] = code[i];
	i = nbytes>>1;
	if (nbytes & 1) Sys_Error("Put_Words given odd nbytes=%d\n",nbytes);
	while (i--) { ch = *cc; *cc = cc[1]; *++cc = ch; cc++; }
	Put_Text(tcode,nbytes);
}
#endif

/* Put_Text -	Write out text to proper portion of file */

put_text(code,length)
	register char *code;
{
	if (pass != 2) return;
	switch(cur_csect_name){
	case C_TEXT:
		fwrite(code, length, 1, tout);  break;
	case C_DATA:
		fwrite(code, length, 1, dout);  break;
	case C_DATA1:
		fwrite(code, length, 1, d1out); break;
	case C_DATA2:
		fwrite(code, length, 1, d2out); break;
	}
	/* else ignore bss segment */
 } /* end Put_Text */


/* set up relocation word for operand:
 *  opnd	pointer to operand structure
 *  size	0 = byte, 1 = word, 2 = long/address
 *  offset	offset into WCode & WReloc array
 */

put_rel(opnd, size, offset, pcrel)
	struct oper *opnd;
	subop_t size;
	long offset;
{
	struct reloc r;
	int tz = (rflag)?0:tsize;
	if (opnd->sym_o == 0) return;	/* no relocation */
	switch( cur_csect_name ){
  	case C_TEXT:
		rtsize += rel_cmd(&r, opnd, size, offset, rtout, pcrel);
		break;
  	case C_DATA:
          	rdsize += rel_cmd(&r, opnd, size, offset - tz, rdout, pcrel);
		break;
	case C_DATA1:
       		rdsize += rel_cmd(&r, opnd, size, offset - tz, rdout, pcrel);
		break;
	case C_DATA2:
		rdsize += rel_cmd(&r, opnd, size, offset - tz, rdout, pcrel);
		break;
	}
	/* else ignore bss segment */
} /* end Put_Rel */


/* rel_cmd -	Generate a relocation command and output */

rel_cmd(rp, opnd, size, offset, file, pcrel)
	register struct reloc* rp;
	struct oper *opnd;
	subop_t size;
	long offset;
	FILE *file;
{ 
	register struct sym_bkt *sp;	/* pointer to symbol */
	static char zed[ sizeof *rp ] = {0};

	sp = opnd->sym_o;
	if (pass == 2) { 
		*rp = *(struct reloc *)zed; /* zero the whole structure */
		/* rp->r.r_extern = 0; */
		if (!(sp->attr_s & S_DEF) && (sp->attr_s & S_EXT)) { 
			rp->r.r_extern = 1;
			rp->rsymbol = sp;
	        }
		else switch( sp->csect_s ){
			case C_TEXT:
				rp->r.r_symbolnum = N_TEXT;
				break;
			default:
	        		rp->r.r_symbolnum = (rflag)?N_TEXT:N_DATA;
				break;
			case C_BSS:
	        		rp->r.r_symbolnum = N_BSS;
				break;
			case C_UNDEF:
				prog_error(E_RELOCATE);
			}
	        rp->r.r_address = offset;
	        rp->r.r_length = (size==SUBOP_L)?2:(size==SUBOP_B)?0:1;
	        rp->r.r_pcrel = pcrel;
	        fwrite(rp, sizeof *rp, 1, file);
     }
     return(sizeof *rp);
}
