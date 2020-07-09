#ifndef TYPES_H_
#define TYPES_H_

typedef union
{
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
	xcb_keysym_t keysym;
	void (*func)
	(
		const Arg*
	);
	const Arg arg;
}
Button;

typedef struct window window;
struct window
{
	xcb_window_t id;
	int_least16_t x, y;
	uint_least16_t width, height;
	uint_least8_t workspace;
	window* next;
};

#endif /* TYPES_H_ */
