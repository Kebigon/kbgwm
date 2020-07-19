#include "xcbutils.h"

#include <stdio.h>
#include <stdlib.h>

extern xcb_connection_t* c;
extern xcb_window_t root;
extern uint16_t numlockmask;

void* emalloc(size_t size)
{
	void* p;

	printf("client size=%zd\n", size);

	if (!(p = malloc(size)))
	{
		printf("Out of memory");
		exit(-1);
	}

	return p;
}

/*
 *
 */

void xcb_register_key_events(Key key)
{
	xcb_keycode_t* keycodes;
	xcb_keycode_t keycode;

	printf("Registering key press event for key %d / key %d\n", key.modifiers, key.keysym);

	keycodes = xcb_get_keycodes(key.keysym);
	uint16_t modifiers[] = { 0, numlockmask, XCB_MOD_MASK_LOCK, numlockmask | XCB_MOD_MASK_LOCK };

	for (int i = 0; (keycode = keycodes[i]) != XCB_NO_SYMBOL; i++)
	{
		for (int j = 0; j != LENGTH(modifiers); j++)
			xcb_grab_key(c, 1, root, key.modifiers | modifiers[j], keycode, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
	}

	free(keycodes);
}

void xcb_register_button_events(Button button)
{
	uint16_t modifiers[] = { 0, numlockmask, XCB_MOD_MASK_LOCK, numlockmask | XCB_MOD_MASK_LOCK };

	for (int i = 0; i != LENGTH(modifiers); i++)
		xcb_grab_button(c, 0, root, BUTTON_EVENT_MASK, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, root, XCB_NONE, button.keysym,
		                button.modifiers | modifiers[i]);
}

/*
 * Get keycodes from a keysym
 * TODO: check if there's a way to keep keysyms
 */
xcb_keycode_t* xcb_get_keycodes(xcb_keysym_t keysym)
{
	xcb_key_symbols_t* keysyms;

	if (!(keysyms = xcb_key_symbols_alloc(c)))
	{
		perror("Not able to retrieve symbols");
		return (NULL);
	}

	xcb_keycode_t* keycode = xcb_key_symbols_get_keycode(keysyms, keysym);

	xcb_key_symbols_free(keysyms);
	return (keycode);
}

/*
 * Get keysym from a keycode
 * TODO: check if there's a way to keep keysyms
 */
xcb_keysym_t xcb_get_keysym(xcb_keycode_t keycode)
{
	xcb_key_symbols_t* keysyms;

	if (!(keysyms = xcb_key_symbols_alloc(c)))
	{
		perror("Not able to retrieve symbols");
		return (0);
	}

	xcb_keysym_t keysym = xcb_key_symbols_get_keysym(keysyms, keycode, 0);

	xcb_key_symbols_free(keysyms);
	return (keysym);
}
