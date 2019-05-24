#!/bin/csh
# @(#)fixedpoint.csh	1.1 (Sun) 2/3/86
# compiler fixed-point test
#
set bin = /usr/src/bin
set cpp = /usr/src/lib/cpp
set libc = /usr/src/lib/libc
set pcc = /usr/src/sun/lib/pcc
set c2 = /usr/src/sun/lib/c2
set as = /usr/src/sun/bin/as
set ld = /usr/src/bin/ld
set echo
#
# step 1: make new components using existing components.
# We must link the cc command with the new library, because
# it uses some routines that are not present in the old one.
#
cd $libc ; make clean ; make
cd $bin ; /bin/cc -c -O -o cc cc.c $libc/libc.a
cd $cpp ; make clean ;  make
cd $pcc ; make clean ;  make
cd $c2 ; make clean ; make
cd $as ; make clean ; make
cd $ld ; make clean ; make
#
# step 2: Install the new components, except for the library.
# The library we have is new but compiled with the old compiler.
#
cd $bin ; install cc /usr/new/cc
cd $cpp ; install cpp /usr/new/cpp
cd $pcc ; install comp /usr/new/ccom
cd $c2 ; install c2 /usr/new/c2
cd $as ; install as /usr/new/as
cd $ld ; install ld /usr/new/ld
#
# step 3: Make a library with the new components
# and install it.  Save a copy of the old library in case
# we want to do code comparisons later.
#
cd $libc ; mv libc.a libc.oldcomp
    make clean ; make CC="/usr/new/cc -tp02a"
    install libc.a /usr/new/libc.a ; ranlib /usr/new/libc.a
#
# step 4: Make new components with the new components
# and link them with the new library.
#
cd $bin ; rm -f cc ; make cc CC="/usr/new/cc -tp02alc"
cd $cpp ; make clean ; make CC="/usr/new/cc -tp02alc"
cd $pcc ; make clean ; make CC="/usr/new/cc -tp02alc"
cd $c2 ; make clean ; make CC="/usr/new/cc -tp02alc"
cd $as ; make clean ; make CC="/usr/new/cc -tp02alc"
cd $ld ; make clean ; make CC="/usr/new/cc -tp02alc"
#
# Now repeat steps 2-4.
# First install new components...
#
cd $bin ; install cc /usr/new/cc
cd $cpp ; install cpp /usr/new/cpp
cd $pcc ; install comp /usr/new/ccom
cd $c2 ; install c2 /usr/new/c2
cd $as ; install as /usr/new/as
cd $ld ; install ld /usr/new/ld
#
# ...make and install a new library
#
cd $libc ; mv libc.a libc.nisei
    make clean ; make CC="/usr/new/cc -tp02a"
    install libc.a /usr/new/libc.a ; ranlib /usr/new/libc.a
#
# ...rebuild the new components
#
cd $bin ; rm -f cc ; make cc CC="/usr/new/cc -tp02alc"
cd $cpp ; make clean ; make CC="/usr/new/cc -tp02alc"
cd $pcc ; make clean ; make CC="/usr/new/cc -tp02alc"
cd $c2 ; make clean ; make CC="/usr/new/cc -tp02alc"
cd $as ; make clean ; make CC="/usr/new/cc -tp02alc"
cd $ld ; make clean ; make CC="/usr/new/cc -tp02alc"
#
# ...and if the results differ, something is broken.
#
cd $bin ; cmp cc /usr/new/cc
cd $cpp ; cmp cpp /usr/new/cpp
cd $pcc ; cmp comp /usr/new/ccom
cd $c2 ; cmp c2 /usr/new/c2
cd $as ; cmp as /usr/new/as
cd $ld ; cmp ld /usr/new/ld
