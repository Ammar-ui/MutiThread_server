CC = gcc
CFLAGS = -c -O2 -Wall -D_REENTRANT

all: web-server

web-server: web-util.o web-server.o web-util.h
	${CC} -o web-server web-util.o web-server.o

web-server.o: web-server.c web-util.h
	${CC} ${CFLAGS} web-server.c

clean:
	rm web-server.o web-server web-server.log
