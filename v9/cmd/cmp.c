#define		BUF		4096

int sflag = 0;
int lflag = 0;
int Lflag = 0;

main(argc, argv)
	char **argv;
{
	register n1, n2, n;
	unsigned char buf1[BUF], buf2[BUF];
	register unsigned char *p, *q;
	int f1, f2, i, nn;
	long nc = 1, o, otoi(), l=1, l1;
	char *name1, *name2, *a;
	unsigned char *b1s, *b1e, *b2s, *b2e;

	for(n=1; n<argc && (*argv[n] == '-'); n++)
		for(a = &argv[n][1]; *a; a++)
			switch(*a) {
			case 's':	sflag = 1; break;
			case 'l':	lflag = 1; break;
			case 'L':	Lflag = 1; break;
			default:	usage();
			}
	if(n > argc-2)
		usage();
	if((f1 = open(name1 = argv[n++], 0)) == -1){
		perror(name1);
		exit(2);
	}
	if((f2 = open(name2 = argv[n++], 0)) == -1){
		perror(name2);
		exit(2);
	}
	if(n < argc){
		o = otoi(argv[n++]);
		if(lseek(f1, o, 0) < 0){
			perror("cmp: lseek by offset1");
			exit(2);
		}
	}
	if(n < argc){
		o = otoi(argv[n++]);
		if(lseek(f2, o, 0) < 0){
			perror("cmp: lseek by offset2");
			exit(2);
		}
	}
	if(n < argc)
		usage();
	b1s = b1e = buf1;
	b2s = b2e = buf2;
	for(;;){
		register unsigned char *ep;

		if(b1s >= b1e){
			if(b1s >= &buf1[BUF])
				b1s = buf1;
			n1 = read(f1, b1s,  &buf1[BUF] - b1s);
			b1e = b1s + n1;
		}
		if(b2s >= b2e){
			if(b2s >= &buf2[BUF])
				b2s = buf2;
			n2 = read(f2, b2s,  &buf2[BUF] - b2s);
			b2e = b2s + n2;
		}
		n = n2;
		if(n1 < n2) n = n1;
		if(n <= 0)
			break;
		if(memcmp(b1s, b2s, n) != 0){
			if(sflag)
				exit(1);
			l1 = 0;
			for(p = b1s, q = b2s, i = 0, nn = n; nn > 0; nn--, p++, q++, i++) {
				if(*p=='\n')
					l1++;
				if(*p != *q){
					if(!lflag){
						printf("%s %s differ: char %d",
						    name1, name2, nc+i);
						printf(Lflag?" line %d\n":"\n", l+l1);
						exit(1);
					}
					printf("%6ld %3o %3o\n", nc+i, *p, *q);
				}
			}
		}		
		if(Lflag)
			for(p = b1s; p < b1e;)
				if(*p++=='\n')
					l++;
		nc += n;
		b1s += n;
		b2s += n;
	}
	if(n1 == n2)
		exit(0);
	if(!sflag)
		printf("EOF on %s\n", (n1 > n2)? name2 : name1);
	exit(1);
}

long
otoi(s)
	register char *s;
{
	long n, ok;

	if(*s == '0'){
		if(s[1] == 'x')
			ok = sscanf(s, "%lx", &n);
		else
			ok = sscanf(s, "%lo", &n);
	} else
		ok = sscanf(s, "%ld", &n);
	if (ok!=1)
		usage();
	return(n);
}

usage()
{
	printf("Usage: cmp [-ls] file1 file2 [offset1 [offset2] ]\n");
	exit(2);
}
