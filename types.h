#ifndef TYPES_H_
#define TYPES_H_

#include <stdbool.h>

typedef union
{
	const bool b;
	const uint_least8_t i;
	const char** cmd;
} Arg;

typedef struct
{
	uint16_t modifiers;
	xcb_keysym_t keysym;
	void (* func)(const Arg*);
	const Arg arg;
} Key;

typedef struct
{
	uint16_t modifiers;
	xcb_button_t keysym;
	void (* func)(const Arg*);
	const Arg arg;
} Button;

typedef struct client_t client;
struct client_t
{
	xcb_window_t id;
	int16_t x, y;
	uint16_t width, height;
	int32_t min_width, min_height;
	int32_t max_width, max_height;
	bool maximized;
	client* previous;
	client* next;
};

#endif /* TYPES_H_ */
