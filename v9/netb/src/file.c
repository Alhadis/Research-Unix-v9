/* when the client reads directories, some work may be required */
#include "share.h"

file tfile;
int roottag;
unsigned char *slash;

openit()
{	int n;
	if(!openfile()) {
		if(client.namiflags != NBROOT)
			client.errno = ENOENT;
		return;
	}
	n = gimmefile();
	if(n < 0)
		return;
	files[n].fd = tfile.fd;
	files[n].flags = tfile.flags;
	if(stat(nmbuf, &files[n].stb) < 0) {
		client.errno = errno;	/* must have been set? */
		close(tfile.fd);
		return;			/* files[n].tag is 0 still */
	}
	checkdev(files[n].stb.st_dev);
	files[n].tag = maketag(&files[n].stb);
	if(setname(n))
		return;
	checkdupl(n);
	retnami(n);
}

creatit()
{	int n, fd;
	if((n = openfile()) && client.flags == NI_NXCREAT) {
		client.errno = EEXIST;
		close(tfile.fd);
		return;
	}
	if(n) {	/* exists, creat it */
		if(writeperm())
			return;
		n = creat(nmbuf, (int)client.mode);
		if(n >= 0)
			close(n);
		else {
			client.errno = errno;
			return;
		}
		n = gimmefile();
		if(n < 0)
			return;
		files[n].fd = tfile.fd;
		files[n].flags = tfile.flags;
		if(stat(nmbuf, &files[n].stb) < 0) {
			client.errno = errno;
			close(tfile.fd);
			return;
		}
		checkdev(files[n].stb.st_dev);
		files[n].tag = maketag(&files[n].stb);
		if(setname(n))
			return;
		checkdupl(n);	/* here it is important too to get the new one */
		retnami(n);
		return;
	}
	/* doesn't exist, creat it */
	if(client.namiflags == NBROOT)
		return;	/* can't creat the root (is errno set?) */
	n = gimmefile();
	if(n < 0)
		return;
	if(dirwriteperm())
		return;
	if((client.mode & S_IFMT)) {	/* no symbolic links or mknods */
		error("client: client.mode 0%o\n", client.mode);
		client.errno = EXDEV;
		return;
	}
	if((fd = creat(nmbuf, client.mode)) < 0) {
		client.errno = errno;
		return;
	}
	if(chown(nmbuf, clientuid(), clientgid()) < 0)
		return;
	files[n].fd = open(nmbuf, 2);
	if(files[n].fd < 0) {
		files[n].fd = open(nmbuf, 0);
		if(files[n].fd < 0) {
			client.errno = errno;
			unlink(nmbuf);
			close(fd);
			return;
		}
		files[n].flags = 1;
	}
	close(fd);	/* fd was write-only */
	if(stat(nmbuf, &files[n].stb) < 0) {	/* it was there a second ago */
		client.errno = errno;
		unlink(nmbuf);
		close(files[n].fd);
		return;
	}
	checkdev(files[n].stb.st_dev);
	files[n].tag = maketag(&files[n].stb);
	if(setname(n))
		return;
	retnami(n);
}
	
linkit()
{	int i, n;
	if(dirwriteperm())
		return;
	if(openfile()) {
		client.errno = EEXIST;
		close(tfile.fd);
		return;
	}
	for(i = 0; i < FILES; i++)
		if(files[i].tag == client.ino)
			break;
	if(i >= FILES) {	/* can't happen */
		error("can't happen: linkit %s, no tag %x\n", nmbuf, client.ino);
		client.errno = EXDEV;	/* not really */
		return;
	}
	debug("linking %s to %s\n", files[i].name, nmbuf);
	n = link(files[i].name, nmbuf);
	if(n < 0) {
		debug("link errno %d\n", errno);
		client.errno = errno;
	}
}

mkdirit()
{	int n;
	if(openfile()) {
		client.errno = EEXIST;
		close(tfile.fd);	/* how does entry get tossed? */
		return;
	}
	if(client.namiflags == NBROOT)
		return;
	if(dirwriteperm())
		return;
	if(mkdir(nmbuf, client.mode) < 0) {
		client.errno = errno;
		return;
	}
	if(chown(nmbuf, clientuid(), clientgid()) < 0)
		return;
}
	
rmdirit()
{
	if(!openfile()) {
		client.errno = EEXIST;
		return;
	}
	close(tfile.fd);
	if(dirwriteperm())
		return;
	if(rmdir(nmbuf) < 0)
		client.errno = errno;
	else
		client.errno = 0;
}

delit()
{
	if(!openfile()) {
		client.errno = ENOENT;
		close(tfile.fd);
		return;
	}
	if(client.namiflags == NBROOT)
		return;
	close(tfile.fd);
	if(dirwriteperm())
		return;
	if((tfile.stb.st_mode & S_IFMT) != S_IFREG) {
		client.errno = EISDIR;
		return;
	}
	if(unlink(nmbuf) < 0)
		client.errno = errno;
	else
		client.errno = 0;
}

openfile()
{	unsigned char *p;
	static struct stat statb;
	int i;
	slash = 0;
	for(p = nmbuf; *p; ) {
		for(; *p == '/'; p++)
			slash = p;
		if(*p == 0)
			break;
		*--p = 0;
		i = stat(nmbuf, &statb);
		if(i < 0) {
			debug("openfile: stat %s (%d)\n", nmbuf, errno);
			client.errno = errno;
			return(0);
		}
		if(searchperm())
			return(0);
		*p++ = '/';
		checkdev(statb.st_dev);
		if(maketag(&statb) == roottag && p[0] == '.' && p[1] == '.'
			&& (p[2] == 0 || p[2] == '/')) {
			client.namiflags = NBROOT;
			client.used = 2 + (p - nmbuf) - nmoffset;
			debug("openfile: root used %d offset %d\n", client.used,
				nmoffset);
			return(0);
		}
		while(*p && *p != '/')
			p++;
	}
	i = stat(nmbuf, &statb);
	if(i < 0)
		return(0);
	tfile.fd = -1;
	tfile.flags = 0;
	tfile.stb = statb;
	tfile.fd = open(nmbuf, 2);
	if(tfile.fd < 0) {
		tfile.fd = open(nmbuf, 0);
		if(tfile.fd < 0)
			return(0);
		tfile.flags = 1;
	}
	return(1);
}

retnami(n)
{	file *t = files + n;
	client.tag = t->tag;
	client.ino = t->stb.st_ino;
	client.dev = hostdev(t->stb.st_dev);
	debug("retnami dev 0x%x ino %d tag 0x%x %s\n", client.dev, client.ino,
		client.tag, t->name);
	client.mode = t->stb.st_mode;
	client.used = 0;	/* ? */
	client.nlink = t->stb.st_nlink;
	client.uid = hostuid(t->stb.st_uid);
	client.gid = hostgid(t->stb.st_gid);
	client.size = t->stb.st_size;		/* watch it! */
	if(cray && client.size >= 0x80000000)
		client.size = 0x7fffffff;
	/* fortunately all known hosts agree on time */
	client.ta = t->stb.st_atime - dtime;
	client.tc = t->stb.st_ctime - dtime;
	client.tm = t->stb.st_mtime - dtime;
}

gimmefile()
{	int i;
	for(i = 0; i < FILES; i++)
		if(files[i].tag == 0)
			break;
	if(i >= FILES) {
		error("out of file structs %d\n", i);
		client.errno = ENFILE;
		return(-1);
	}
	files[i].flags = 0;
	files[i].pos = 0;
	return(i);
}

/* is this really an error to recover from? */
setname(n)
{	unsigned char *p;
	p = (unsigned char *)malloc(strlen(nmbuf) + 1);
	if(p == NULL) {
		error("out of space on %s\n", nmbuf);
		client.errno = ENOSPC;
		return(-1);
	}
	strcpy(p, nmbuf);
	files[n].name = p;
	return(0);
}

checkdupl(n)
{	int i;
	unsigned char *x;
	/* if it duplicates someone we've got, toss the old one, but use its tag */
	for(i = 0; i < FILES; i++) {
		if(i == n)
			continue;
		if(files[i].tag != files[n].tag)
			continue;
		debug("ok: creat found dup %s\n", nmbuf);
		if(strlen(files[i].name) < strlen(files[n].name)) {
			x = files[n].name;
			files[n].name = files[i].name;
			free(x);
		}
		else
			free(files[i].name);
		close(files[i].fd);
		files[i].tag = 0;
	}
	/* update device translation if necessary */
	checkdev(files[n].stb.st_dev);
}

dev *devs;
int devlen, ndev;
checkdev(n)
{	int i;
	for(i = 0; i < ndev; i++)
		if(devs[i].hdev == n)
			return;
	if(ndev >= devlen) {
		if(!devlen)
			devs = (dev *) malloc((devlen = 10) * sizeof(dev));
		else {
			devlen *= 2;
			devs = (dev *) realloc((char *)devs, devlen * sizeof(dev));
			error("reallocated devs to %d entries\n", devlen);
		}
		if(!devs)
			fatal("alloc of %d devs failed!\n", devlen);
	}
	if(ndev >= 256)
		fatal("%d devs? (too many)\n", ndev);
	devs[ndev].hdev = n;
	devs[ndev].cdev = hisdev | ndev;
	debug("devs[%d] %x->%x\n", ndev, n, hisdev | ndev);
	ndev++;
}

addroot()
{	int n;
	n = gimmefile();
	files[n].name = (unsigned char *)"/";
	files[n].fd = open("/", 2);
	if(files[n].fd < 0) {
		files[n].fd = open("/", 0);
		if(files[n].fd < 0)
			fatal("can't open root, errno %d\n", errno);
		files[n].flags = 1;
	}
	if(stat("/", &files[n].stb) < 0)
		fatal("stat root errno %d\n", errno);
	checkdev(files[n].stb.st_dev);
	files[n].tag = maketag(&files[n].stb);
	roottag = files[n].tag;
	debug("root %d, tag 0x%x\n", n, files[n].tag);
	if(files[n].stb.st_ino != ROOTINO)	/* which ROOTINO is this? */
		error("client and host probably don't agree on rootino %d\n",
			files[n].stb.st_ino);
}

maketag(s)
struct stat *s;
{	int tag;
	tag = (hostdev(s->st_dev) << 16) | s->st_ino;
	debug("maketag (%x,%x)->%x\n", s->st_dev, s->st_ino, tag);
	return(tag);
}

#if cray == 1
#include "signal.h"
int crapsig;
char sysbuf[256];
mkdir(s)
char *s;
{	int pid, ret = 0;
	if(!crapsig++)
		signal(SIGCLD, SIG_IGN);
	sprintf(sysbuf, "mkdir %s", s);
	if(strlen(sysbuf) >= sizeof(sysbuf))
		fatal("sysbuf overflow %s\n", sysbuf);
	if((pid = fork()) == 0) {
		setuid(clientuid());
		setgid(clientgid());
		exit(system(sysbuf));
	}
	wait(&ret);
	if(!ret)
		return(0);
	errno = EPERM;	/* who knows? */
	return(-1);
}
rmdir(s)
char *s;
{
	if(!crapsig++)
		signal(SIGCLD, SIG_IGN);
	sprintf(sysbuf, "rmdir %s", s);
	if(strlen(sysbuf) >= sizeof(sysbuf))
		fatal("sysbuf overflow in rmdir %s\n", sysbuf);
	if(system(sysbuf) == 0)
		return(0);
	errno = EPERM;	/* ? */
	return(-1);
}
fchmod()
{
	fatal("!!!fchmod called\n");
}
#endif
