#ifndef TYPES_H_
#define TYPES_H_

typedef union {
	const char** cmd;
} Arg;

typedef struct {
	uint16_t modifiers;
	xcb_keysym_t keysym;
	void (* func)(const Arg*);
	const Arg arg;
} Key;

#endif /* TYPES_H_ */
