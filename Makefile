CC = gcc
CFLAGS = -Wall -g -pthread
PROGRAMS = client server

ALL: ${PROGRAMS}

client: client.c
	${CC} ${CFLAGS} -o client client.c rtp.c
	
server: server.c
	${CC} ${CFLAGS} -o server server.c rtp.c

clean:
	rm -f ${PROGRAMS}
