CC = g++
CFLAGS =-g -I ./ -pthread -D_REENTRANT -Wall #-O2
CLEANFILES = core core.* *.core *.o temp.* *.out typescript* *.lc *.lh *.bsdi *.sparc *.uw
LDFLAGS=`pkg-config --cflags --libs opencv`

PROGS =	picserv

all:	${PROGS}

picserv:
	${CC} ${CFLAGS} picserv.cpp serv.h serv.cpp $(LDFLAGS) -o s

piccli:
	${CC} ${CFLAGS} piccli.cpp -o c

clean:
	rm -f ${PROGS} ${CLEANFILES}
