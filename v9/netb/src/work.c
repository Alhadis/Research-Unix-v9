#include "share.h"

file files[FILES];	/* dynamic? */
int nfile = FILES;
int nmoffset;

doput()
{	int i;
	for(i = 0; i < nfile; i++)
		if(files[i].tag == client.tag)
			break;
	if(i >= nfile) {
		error("put: tag 0x%x not found\n", client.tag);
		client.errno = ENOENT;
		return;
	}
	if(files[i].tag == roottag)	/* never toss the root */
		return;
	close(files[i].fd);
	files[i].fd = -1;
	files[i].tag = 0;
	free(files[i].name);
}

/* real paranoia would check that name and fd still refer to the same file */
doupdate()
{	int i,f = 0;
	for(i = 0; i < nfile; i++)
		if(files[i].tag == client.tag)
			break;
	if(i >= nfile) {
		error("update: tag 0x%x not found\n", client.tag);
		client.errno = ENOENT;
		return;
	}
	errno = 0;
	if(client.ta)
		f++, client.ta += dtime;
	else
		client.ta = files[i].stb.st_atime;
	if(client.tm)
		f++, client.tm += dtime;
	else
		client.tm = files[i].stb.st_mtime;
	if(f)	/* could someone have changed the file name underfoot? */
		utime(files[i].name, &client.ta);
	if(errno) {
		client.errno = errno;
		error("update: utime errno %d\n", errno);
		/* and forget the rest of it */
		return;
	}
	if(client.mode != files[i].stb.st_mode) {
		if(!isowner(&files[i].stb)) {
			/* except for client root, this should never happen */
			error("doupdate: client user %d not owner of %s (chmod)\n",
				client.uid, files[i].name);
			client.errno = EPERM;
			return;
		}
		if(cray == 1) {	/* stupid thing has no fchmod */
			chmod(files[i].name, client.mode);
			if(errno) {
				error("chmod(%s,0%o) failed %d\n", files[i].name,
					client.mode, errno);
				return;
			}
		} else {
			fchmod(files[i].fd, client.mode);
			if(errno) {
				error("fchmod(%s,0%o) failed %d\n", files[i].name,
					client.mode, errno);
				return;
			}
		}
	}
	/* refresh the stat? */
}

doread()
{	int i, n;
	for(i = 0; i < nfile; i++)
		if(client.tag == files[i].tag)
			break;
	if(i >= nfile) {
		error("doread: tag 0x%x missing\n", client.tag);
		client.errno = ENOENT;
		return;
	}
	/* permissions check missing here */
	/* trust the client, except for unknown users, and our policy on `other' */
	/*if(clientuid() == -1  && !otherok) {
		client.errno = EPERM;
		return;
	}*/ /*no point, can't get here */
	/* if we're doing read-ahead, here's the spot */
	if((files[i].stb.st_mode & S_IFMT) == S_IFDIR) {
		/* fancy directory handling would go here
		doreaddir(i);
		return;*/
	}
	/* now we know that the headers for sndread add up to 16 bytes */
	if(files[i].pos != client.offset) {
		lseek(files[i].fd, client.offset, 0);
		files[i].pos = client.offset;	/* are you sure? */
	}
	if(16 + client.count > inlen)	/* sanity */
		client.count = inlen - 16;
	n = read(files[i].fd, inbuf + 16, (int)client.count);
	if(n < 0) {
		files[i].pos = (unsigned int)-1;	/* can't trust it? */
		client.errno = errno;
		return;
	}
	client.resplen = 16 + n;	/* read sets it */
	files[i].pos += n;
}

dowrite(p)
unsigned char *p;
{	int i, n;
	for(i = 0; i < nfile; i++)
		if(client.tag == files[i].tag)
			break;
	if(i >= nfile) {
		debug("dowrite: tag 0x%x missing\n", client.tag);
		client.errno = ENOENT;
		return;
	}
	/*if(clientuid() == -1  && !otherok) {
		client.errno = EPERM;
		return;
	}*/ /* no point, can't happen */
	if(files[i].flags) {
		client.errno = EPERM;
		return;
	}
	if(files[i].pos != client.offset) {
		lseek(files[i].fd, client.offset, 0);
		files[i].pos = client.offset;
	}
	if(24 + client.count > inlen) {
		client.errno = EFAULT;
		return;
	}
	n = write(files[i].fd, inbuf + 24, (int)client.count);	/* secret 24 */
	if(n != client.count) {
		client.errno = errno? errno: EIO;
		files[i].pos = (unsigned int) -1;	/* ??? */
		return;
	}
	files[i].pos += i;
}

donami(p, len)
unsigned char *p;
{	int i, offset;
	unsigned char *s, *t, *bufend;
	for(i = 0; i < nfile; i++)
		if(client.tag == files[i].tag)
			break;
	if(i >= nfile) {
		error("nami: tag 0x%x not found\n", client.tag);
		client.errno = ENOENT;
		return;
	}
	debug("nbnami: tag 0x%x for %d\n", client.tag, i);
	/* build the new name in nmbuf */
	for(s = nmbuf, t = files[i].name; *t; )
		*s++ = *t++;
	*s++ = '/';
	nmoffset = s - nmbuf;
	bufend = s + len;
	for(t = p; *t && s < bufend; t++) {	/* two tests for paranoia */
		if(s >= nmbuf + inlen) {
			fatal("nami overran nmbuf\n");
			client.errno = EFAULT;
			return;
		}
		*s++ = *t;
	}
	*s = 0;
	debug("nbnami: nmbuf |%s|\n", nmbuf);
	/* first write the simple code and then worry about . and .. and null */
	if(client.flags == 0) {
		openit();
		return;
	}
	switch(client.flags) {
	default:
		error("nami: unk flag %d\n", client.flags);
		client.errno = EINVAL;
		return;
	case NI_NXCREAT:	/* expects it not to exist */
		creatit();
		break;
	case NI_LINK:	/* expects it not to exist */
		linkit();
		break;
	case NI_MKDIR:	/* expects it not to exist */
		mkdirit();
		break;
	case NI_RMDIR:	/* expects it to exist */
		rmdirit();
		break;
	case NI_DEL:	/* expects it to exist */
		delit();
		break;
	case NI_CREAT:	/* doesn't care */
		creatit();
		break;
	}
}

/* are fd and name consistent? (when might they not be?) */
/* this is only called on fstat() or stats of the root */
dostat()
{	static int t = 0;
	int i, n;
	struct stat *x;
	if(++t % 3 == 1 || t < 3) {
		i = time(0) - client.ta;
/*		if(i != dtime)
			error("client time %d host time %d dtime %d\n",
				client.ta, time(0), i);
*/
		dtime = i;
	}
	for(i = 0; i < nfile; i++)
		if(client.tag == files[i].tag)
			break;
	if(i >= nfile) {
		error("dostat: tag %d not found\n", client.tag);
		client.errno = ENOENT;
		return;
	}
	n = fstat(files[i].fd, &files[i].stb);
	if(n < 0) {	/* this is probably a serious error */
		error("dostat: returned errno %d %s(%d) %x\n", errno, files[i].name,
			files[i].fd, files[i].tag);
		client.errno = errno;
		return;
	}
	x = &files[i].stb;
	client.ino = x->st_ino;
	client.dev = hostdev(x->st_dev);
	client.nlink = x->st_nlink;
	client.uid = hostuid(x->st_uid);
	client.gid = hostgid(x->st_gid);
	client.mode = x->st_mode;	/* permission calculation? */
	client.size = x->st_size;
	client.ta = x->st_atime - dtime;
	client.tm = x->st_mtime - dtime;
	client.tc = x->st_ctime - dtime;
	if(cray && client.size >= 0x80000000)
		client.size = 0x7fffffff;
	if((client.mode & S_IFMT) != S_IFREG && (client.mode & S_IFMT) != S_IFDIR)
		client.mode = 0;
}
	
/* paranoia suggests checking that fd and name are consistent */
dotrunc()
{	int i, n;
	for(i = 0; i < nfile; i++)
		if(client.tag == files[i].tag)
			break;
	if(i >= nfile) {
		error("trunc: tag %d not found\n", client.tag);
		client.errno = ENOENT;
		return;
	}
	if(clientuid() == -1 && !otherok) {
		client.errno = EPERM;
		return;
	}
	n = creat(files[i].name, files[i].stb.st_mode);
	if(n < 0) {
		error("dotrunc: creat errno %d %s\n", errno, files[i].name);
		client.errno = errno;
		return;
	}
	close(n);
}
