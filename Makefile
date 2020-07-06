OBJ = kbgwm.o xcbutils.o

CFLAGS+=-g -std=c99 -Wall -Wextra -I/usr/local/include
LDFLAGS+=-L/usr/local/lib -lxcb -lxcb-keysyms

all: clean format kbgwm

kbgwm: ${OBJ}
	${CC} ${CFLAGS} ${OBJ} ${LDFLAGS} -o $@

kbgwm.o: kbgwm.c
xcbutils.o: xcbutils.c

clean:
	rm -f kbgwm ${OBJ}