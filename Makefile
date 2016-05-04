CFLAGS=-Wall -std=c11 -pedantic
bridge:bridge.c station.c
	${CC} ${CFLAGS} -pthread bridge.c -o bridge -g
	${CC} ${CFLAGS} station.c -o station -g
