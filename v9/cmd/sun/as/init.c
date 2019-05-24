#ifndef lint
static	char sccsid[] = "@(#)init.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "as.h"

char title[STR_MAX];
char o_outfile = 0;		/* 1 if .rel file name is specified by user */
int pass = 0;			/* which pass we're on */
int implicit_cpid = INITIAL_CPID ; /* Default coprocessor id. */
int current_cpid ;		/* Coprocessor for current instruction. */
char rel_name[STR_MAX];		/* Name of .rel file */
FILE *rel_file;			/* and ptr to it */
struct sym_bkt *dot_bkt ;	/* Ptr to location counter's symbol bucket */
long tsize = 0;			/* sizes of three main csects */
long dsize = 0;
long d1size = 0;
long d2size = 0;
long bsize = 0;
struct ins_ptr *ins_hash_tab[HASH_MAX];
struct ins_ptr *inst;
char *source_name[FILES_MAX];
FILE *source_file[FILES_MAX];
int file_count= 0, current_file= 0;
char *asm_name;	/* set from argv[0] below: used in error.c */
int errors = 0;		/* Number of errors in this pass */
char file_name[STR_MAX];
char o_lflag = 0;
int  d2flag = 0;	/* all offsets are 16 bits */
int  rflag  = 0;	/* data goes at end of text segment (read-only) */
int  Oflag  = 0;        /* sdi optimization : over the whole file */
int  jsrflag = 0,	/* all "long" jumps use 16-bit displacements */
     Jflag   = 0,	/* all jumps are "long" */
     even_align_flag = 0, /* 0 = align on 4 byte boundaries; 1 = align on 2 bytes */
     hflag   = 0;	/* all calls use 4 byte displacement. other jumps are "long" */
#ifdef EBUG
int  debflag = 0;
#endif

extern int is68020();	/* undocumented hack from libc.a */
int ext_instruction_set; 

init(argc,argv)
	char *argv[];
{
	char *strncpy();
	char *cp1, *cp2, *end, *rindex();

	ext_instruction_set = is68020();
	asm_name = *(argv++);
	while (--argc) {
	  if (argv[0][0] == '-') switch (argv[0][1]) {
	    case 'o':	o_outfile++;
			strcpy(rel_name, argv[1]);
			argv++;			
			argc--;
			break;

	    case 'L':	
			o_lflag++;
			break;
	    case 'R':
			rflag++;
			break;
	    case 'h':
			if (Oflag) goto Owarn;
			if (Jflag && !hflag) goto Jwarn;
			hflag++;
			Jflag++;
			break;
	    case 'j':
			jsrflag++;
			Oflag++;
			break;
	    case 'J':
			Jflag++;
			if (Oflag) {
				fprintf(stderr,"Warning: -J overrides -O\n");
				Oflag = 0;
			}
			if (hflag) {
	    Jwarn:		fprintf(stderr,"Warning: -J overrides -h\n");
				hflag = 0;
			}
			break;
	    case 'O':
			if (Jflag) {
				fprintf(stderr,"Warning: -J overrides -O\n");
				Oflag = 0;
				break;
			}
			if (hflag) {
	    Owarn:		fprintf(stderr,"Warning: -h overrides -O\n");
				Oflag = 0;
				break;
			}
			Oflag++;
			break;
	    case '1':
			if (argv[0][2] == '0'){
				ext_instruction_set=0;
				break;
			}
			goto oops;
	    case '2':
			if (argv[0][2] == '0'){
				ext_instruction_set=1;
				break;
			}
			goto oops;
	    case 'm':
			/*
			 * this is redundant but consistent with
			 * cc, f77, and pc
			 */
			if (!strcmp(argv[0]+1,"m68010")) {
				ext_instruction_set=0;
				break;
			}
			if (!strcmp(argv[0]+1,"m68020")) {
				ext_instruction_set=1;
				break;
			}
			goto oops;
	    case 'e':
			even_align_flag = 1 ;
			break ;
#ifdef EBUG
	    case 'D':
			debflag++;
			break;
#endif
	    case 'd':
			if (argv[0][2] == '2'){
				d2flag++;
				Oflag++;
				break;
			}
			/* else fall through */

	    oops:
	    default:	fprintf(stderr,"%s: Unknown option '%c' ignored.\n",
				asm_name, argv[0][1]);
	  } else {
		if (file_count >= FILES_MAX) {
			fprintf(stderr, "Too many source files given (max=%d)\n", FILES_MAX); exit(99);};
		source_name[file_count] = argv[0];
		strcpy(file_name, argv[0]);
		if ((source_file[file_count]= fopen(file_name,"r")) == NULL) {
		    /* open source file */
		    if ((end = rindex(source_name[file_count], '.')) == 0 || strcmp(end, ".a68") != 0) {
			    fprintf(stderr,"%s: Can't open source file: %s\n", asm_name, file_name);
			    exit(1);
		    }
			 strncpy(file_name, argv[0], STR_MAX);
			 if ((source_file[file_count]= fopen(file_name,"r")) == NULL) {
			     fprintf(stderr,"Can't open source file: %s\n",file_name);
			     exit(1);
			 }}
		file_count++;
	  }
	  argv++;
	}

	if (file_count == 0){
		fprintf(stderr,"%s: No input file\n", asm_name);
		exit( 99 );
	}


/* Check to see if we can open output file */
	if(!o_outfile) {
		strcpy(rel_name, "a.out");
	}
	if ((rel_file = fopen(rel_name,"w")) == NULL){
		printf("%s: Can't create output file: %s\n",asm_name,rel_name);
		exit(1);
	}
	fclose(rel_file);	/* rel_header will open properly */

	sym_init(); 		/* Initialize symbols */
	dot_bkt = lookup(".");		/* make bucket for location counter */
	dot_bkt->csect_s = cur_csect_name;
	dot_bkt->attr_s = S_DEC | S_DEF | S_LABEL; 	/* "S_LABEL" so it cant be redefined as a label */
	init_regs();			/* define register names */
	d_ins();			/* set up opcode hash table */
	perm();
	start_pass();
}

