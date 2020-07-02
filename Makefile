SRC = kbgwm.c
OBJ = kbgwm.o

CFLAGS+=-g -std=c99 -Wall -Wextra -I/usr/local/include
LDFLAGS+=-L/usr/local/lib -lxcb -lxcb-keysyms

kbgwm: ${OBJ}
	${CC} ${CFLAGS} ${OBJ} ${LDFLAGS} -o $@

kbgwm.o: kbgwm.c config.h

clean:
	rm -f kbgwm ${OBJ}
