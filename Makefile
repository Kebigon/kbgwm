OBJ = kbgwm.o xcbutils.o events.o client.o

CFLAGS+=-g -std=c99 -Wall -Wextra -pedantic -Wstrict-overflow -fno-strict-aliasing -I/usr/local/include -march=native
LDFLAGS+=-L/usr/local/lib -lxcb -lxcb-icccm -lxcb-keysyms

all: clean kbgwm

kbgwm: ${OBJ}
	${CC} ${CFLAGS} ${OBJ} ${LDFLAGS} -o $@

kbgwm.o: kbgwm.c
xcbutils.o: xcbutils.c

clean:
	rm -f kbgwm ${OBJ}

format:
	clang-format -i -style=file *.{c,h}

check:
	cppcheck --enable=all --inconclusive --std=c99 --platform=unix64 *.{c,h}
