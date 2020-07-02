#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>

#define LENGTH(X) (sizeof X / sizeof X[0])

typedef struct {
	uint16_t modifiers;
	xcb_keysym_t key;
} Key;

#include "config.h"

bool quit = false;
xcb_connection_t *c;
xcb_window_t root;

void setupEvents()
{
	/*
	 * Get all the key symbols
	 */
	xcb_key_symbols_t *syms = xcb_key_symbols_alloc(c);

	for (int i = 0; i != LENGTH(keys); i++)
	{
		xcb_keycode_t *keycode = xcb_key_symbols_get_keycode(syms, keys[i].key);

		xcb_grab_key(c, 1, root, keys[i].modifiers, keycode,
		             XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

		free(keycode);
	}

	xcb_key_symbols_free(syms);
}

void eventLoop()
{
	xcb_generic_event_t *event;

	while (!quit)
	{
		event = xcb_wait_for_event(c);

		switch (event->response_type & ~0x80)
		{
		}

		free(event);
	}
}

int main(void)
{
	/*
	 * displayname = NULL -> use DISPLAY environment variable
	 */
	int screenp;
	c = xcb_connect(NULL, &screenp);
	if (xcb_connection_has_error(c))
	{
		perror("xcb_connect");
		exit(1);
	}

	/*
	 * Find the screen
	 */
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(c));
	for (int i = 0; i < screenp; ++i)
	{
		xcb_screen_next(&iter);
	}

	xcb_screen_t *screen = iter.data;
	if (!screen)
	{
		xcb_disconnect(c);
		perror("screen not found");
		exit(1);
	}

	root = screen->root;

	xcb_flush(c);

	setupEvents();

	// Event loop
	eventLoop();

	xcb_disconnect(c);

	return 0;
}
