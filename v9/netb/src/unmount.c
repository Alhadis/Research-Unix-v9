main(argc, argv)
char **argv;
{	extern int errno;
	int i;
	for(i = 1; i < argc; i++) {
/*		funmount(argv[1]);*/
		syscall(50, argv[1]);
		perror(argv[1]);
	}
	exit(0);
}
