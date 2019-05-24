nothing
2500 12800
d--777 0 0
bin	d--775 3 4
	adb	---755 3 4 test/bin/adb
	ar	---755 3 4 test/bin/ar
	as	---755 3 4 test/bin/as
	cat	---755 3 4 test/bin/cat
	cc	---755 3 4 test/bin/cc
	chmod	---755 3 4 test/bin/chmod
	cmp	---755 3 4 test/bin/cmp
	cp	---755 3 4 test/bin/cp
	date	---755 3 4 test/bin/date
	dd	---755 3 4 test/bin/dd
	df	--g755 0 2 test/bin/df
	diff	---755 3 4 test/bin/diff
	du	---755 3 4 test/bin/du
	ed	---755 3 4 test/bin/ed
	echo	---755 3 4 test/bin/echo
	false	---755 3 4 test/bin/false
	grep	---755 3 4 test/bin/grep
	kill	---755 3 4 test/bin/kill
	ld	---755 3 4 test/bin/ld
	ln	---755 3 4 test/bin/ln
	ls	---755 3 4 test/bin/ls
	make	---755 3 4 test/bin/make
	mkdir	---755 3 4 test/bin/mkdir
	mt	---755 3 4 test/bin/mt
	mv	-u-755 0 4 test/bin/mv
	newgrp	-u-755 0 4 test/bin/newgrp
	nice	---755 3 4 test/bin/nice
	nm	---755 3 4 test/bin/nm
	passwd	--g755 3 4 test/bin/passwd
	pr	-u-755 0 4 test/bin/pr
	ps	-u-755 0 4 test/bin/ps
	pwd	---755 3 4 test/bin/pwd
	ranlib	---755 3 4 test/bin/ranlib
	rm	---755 3 4 test/bin/rm
	rmdir	---755 3 4 test/bin/rmdir
	sh	---755 3 4 test/bin/sh
	size	---755 3 4 test/bin/size
	stty	---755 3 4 test/bin/stty
	sync	---755 3 4 test/bin/sync
	tar	---755 3 4 test/bin/tar
	test	---755 3 4 test/bin/test
	time	---755 3 4 test/bin/time
	true	---755 3 4 test/bin/true
	vmstat	---755 3 4 test/bin/vmstat
	who	---755 3 4 test/bin/who
	write	---755 3 4 test/bin/write
	$
cs	d--775 3 4
	$
dev	d--777 0 0
	ttya	c--666 0 1 12 0
	ttyb	c--666 0 1 12 1
	console	c--644 0 1 12 2
	mouse	c--666 0 1 12 3
	stdin	c--666 0 1 40 0
	stdout	c--666 0 1 40 1
	stderr	c--666 0 1 40 2
	tty	c--666 0 1 40 3
	drum	c--666 0 0 7 0
	mem	c--644 0 2 3 0
	kmem	c--644 0 2 3 1
	kmemr	c--444 0 4 3 12
	null	c--666 0 2 3 2
	eeprom	c--644 0 2 3 11
	fb	c--666 0 1 28 0
	rsd0a	c--640 0 2 17 0
	rsd0b	c--640 0 2 17 1
	rsd0c	c--640 0 2 17 2
	rsd0g	c--640 0 2 17 6
	sd0a	b--640 0 2 7 0
	sd0b	b--640 0 2 7 1
	sd0c	b--640 0 2 7 2
	sd0g	b--640 0 2 7 6
	dk	d--755 0 1
		$
	pt	d--755 0 1
		$
	Makedev	---755 0 4 test/dev/Makedev
	$
etc	d--775 0 4
	chown	---755 3 4 test/etc/chown
	config	---755 3 4 test/etc/config
	cron	---755 0 4 test/etc/cron
	crontab	---644 0 4 test/etc/crontab
	fsck	---755 0 4 test/etc/fsck
	fstab	---644 0 4 test/etc/fstab
	getty	---755 0 4 test/etc/getty
	group	---644 0 0 test/etc/group
	halt	---755 0 4 test/etc/halt
	init	---755 0 0 test/etc/init
	login	---755 0 4 test/etc/login
	mkfs	---755 0 4 test/etc/mkfs
	mklost+found	---755 0 4 test/etc/mklost+found
	mknod	---755 0 4 test/etc/mknod
	mount	---755 0 4 test/etc/mount
	passwd	---644 0 0 test/etc/passwd
	procmount	---755 0 4 test/etc/procmount
	rc	---644 0 4 test/etc/rc
	reboot	---755 0 4 test/etc/reboot
	shutdown	---755 0 4 test/etc/shutdown
	su	-u-755 0 4 test/etc/su
	ttys	---644 0 4 test/etc/ttys
	umount	---755 0 4 test/etc/umount
	update	---755 0 4 test/etc/update
	wall	---755 0 4 test/etc/wall
	whoami	---755 0 4 test/etc/whoami
	$
lib	d--775 3 4
	c2	---755 3 4 test/lib/c2
	ccom	---755 3 4 test/lib/ccom
	cpp	---755 3 4 test/lib/cpp
	crt0.o	---755 3 4 test/lib/crt0.o
	dst	---644 4 4 test/lib/dst
	libc.a	---644 3 4 test/lib/libc.a
	libC.a	---644 3 4 test/lib/libC.a
 	$
mnt	d--755 0 1
	$
n	d--755 0 4
	capek	d--755 0 4
		$
	daksun	d--755 0 4
		$
	labsun	d--755 0 4
		$
	lucian	d--755 0 4
		$
	visiona	d--755 0 4
		$
	$
proc	d--777 0 1
	$
tmp	d--777 0 1
	$
usr	d--755 0 1
	$
.profile	---644 0 1 test/.profile
startup		---755 0 4 test/startup
boot		---755 0 3 test/boot
unix		---755 0 4 test/unix.75
$
