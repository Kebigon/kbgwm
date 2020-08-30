/*
 * kbgwm, a sucklessy floating window manager
 * Copyright (C) 2020 Kebigon
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "xcbutils.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xproto.h>

extern xcb_connection_t* c;
extern xcb_window_t root;
extern uint16_t numlockmask;
extern xcb_atom_t wm_protocols;

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

#define ONLY_IF_EXISTS 0

/* Get a defined atom from the X server. */
xcb_atom_t xcb_get_atom(const char* atom_name)
{
	xcb_intern_atom_cookie_t cookie = xcb_intern_atom(c, ONLY_IF_EXISTS, strlen(atom_name), atom_name);

	xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(c, cookie, NULL);

	/* XXX Note that we return 0 as an atom if anything goes wrong.
	 * Might become interesting.*/

	if (reply == NULL)
		return 0;

	xcb_atom_t atom = reply->atom;
	free(reply);
	return atom;
}

bool xcb_send_atom(client* client, xcb_atom_t atom)
{
	assert(client != NULL);

	bool supported = false;
	xcb_get_property_cookie_t cookie = xcb_icccm_get_wm_protocols_unchecked(c, client->id, wm_protocols);

	// Get the supported atoms for this client
	xcb_icccm_get_wm_protocols_reply_t protocols;
	if (xcb_icccm_get_wm_protocols_reply(c, cookie, &protocols, NULL) == 1)
	{
		for (uint_fast32_t i = 0; i < protocols.atoms_len; i++)
		{
			if (protocols.atoms[i] == atom)
			{
				supported = true;
				break;
			}
		}
	}

	// The client does not support WM_DELETE, let's kill it
	if (!supported)
		return false;

	xcb_client_message_event_t ev;
	ev.response_type = XCB_CLIENT_MESSAGE;
	ev.format = 32;
	ev.sequence = 0;
	ev.window = client->id;
	ev.type = wm_protocols;
	ev.data.data32[0] = atom;
	ev.data.data32[1] = XCB_CURRENT_TIME;
	xcb_send_event(c, false, client->id, XCB_EVENT_MASK_NO_EVENT, (char*) &ev);
	return true;
}
