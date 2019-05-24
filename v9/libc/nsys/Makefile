CC	=ncc
CFLAGS	=-O -I/n/labsun/src/h

OFILES=	_exit.o _signal.o access.o acct.o alarm.o cerror.o chdir.o \
	chmod.o chown.o chroot.o close.o creat.o dup.o execl.o execle.o \
	execv.o execve.o fchmod.o fchown.o fmount.o fork.o \
	fstat.o funmount.o getgid.o getgroups.o getlog.o getpid.o \
	getuid.o ioctl.o kill.o limits.o link.o lseek.o lstat.o \
	mkdir.o mknod.o mount.o nap.o nice.o open.o pipe.o prof.o \
	read.o readlink.o reboot.o rmdir.o sbrk.o select.o setgid.o \
	setgroups.o setpgrp.o setruid.o settod.o setuid.o signal.o \
	sigtramp.o stat.o stime.o swapon.o symlink.o sync.o syscall.o \
	time.o times.o umask.o umount.o unlink.o utime.o vadvise.o \
	vlimit.o vtimes.o wait.o wait3.o write.o setjmp.o \
	mmap.o munmap.o getpagesize.o rtproc.o rtsched.o

all: $(OFILES)

install: $(OFILES)
	mv $(OFILES) ../OBJ
