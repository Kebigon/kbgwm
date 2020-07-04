#include "xcbutils.h"

#include <stdio.h>
#include <stdlib.h>

extern xcb_connection_t* c;
extern xcb_window_t root;

/*
 *
 */

void xcb_register_key_press_events(Key key)
{
	xcb_keycode_t* keycodes;
	xcb_keycode_t keycode;

	printf("Registering key press event for key %d / key %d\n", key.modifiers, key.keysym);

	keycodes = xcb_get_keycodes(key.keysym);

	for (int i = 0; (keycode = keycodes[i]) != XCB_NO_SYMBOL; i++)
		xcb_grab_key(c, 1, root, key.modifiers, keycode,
		             XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

	free(keycodes);
}

/*
 * Get keycodes from a keysym
 * TODO: check if there's a way to keep keysyms
 */
xcb_keycode_t* xcb_get_keycodes(xcb_keysym_t keysym)
{
	xcb_key_symbols_t* keysyms;

	// Not able to retrieve the keysyms somehow
	if (!(keysyms = xcb_key_symbols_alloc(c)))
		return NULL;

	xcb_keycode_t* keycode = xcb_key_symbols_get_keycode(keysyms, keysym);

	xcb_key_symbols_free(keysyms);
	return keycode;
}

/*
 * Get keysym from a keycode
 * TODO: check if there's a way to keep keysyms
 */
xcb_keysym_t xcb_get_keysym(xcb_keycode_t keycode)
{
	xcb_key_symbols_t* keysyms;

	if (!(keysyms = xcb_key_symbols_alloc(c)))
		return 0;

	xcb_keysym_t keysym = xcb_key_symbols_get_keysym(keysyms, keycode, 0);

	xcb_key_symbols_free(keysyms);
	return keysym;
}
