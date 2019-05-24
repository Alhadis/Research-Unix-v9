OBJS=main.o dkctlchan.o dial.o listen.o
COBJS=main.O dkctlchan.O dial.O listen.O
MGR=dk
INCS=-I../../h
CFLAGS=-g ${INCS}
LIBS=../common/libcommon.a ../../libipc/libipc.a
CLIBS=../common/*.O ../../libipc/*.O

all: cyntax ${MGR}mgr

${MGR}mgr: ${OBJS}
	${CC} -g ${OBJS} -o ${MGR}mgr ${LIBS}

%.O: %.c ../../h/defs.h ../../h/ipc.h
	cyntax -c $(INCS) $%.c 

cyntax: ${COBJS}
	cyntax ${COBJS} ${CLIBS}

clean:
	rm -f *.[oO] ${MGR}mgr
