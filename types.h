#ifndef TYPES_H_
#define TYPES_H_

#include <stdbool.h>

typedef union
{
	const bool b;
	const uint_least8_t i;
	const char** cmd;
}
Arg;

typedef struct
{
	uint16_t modifiers;
	xcb_keysym_t keysym;
	void (*func)
	(
		const Arg*
	);
	const Arg arg;
}
Key;

typedef struct
{
	uint16_t modifiers;
	xcb_button_t keysym;
	void (*func)
	(
		const Arg*
	);
	const Arg arg;
}
Button;

typedef struct clientstruct client;
struct clientstruct
{
	xcb_window_t id;
	int_least16_t x, y;
	uint_least16_t width, height;
	client* previous;
	client* next;
};

#endif /* TYPES_H_ */
