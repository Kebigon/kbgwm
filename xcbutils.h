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

#ifndef KBGWM_XCBUTILS_H
#define KBGWM_XCBUTILS_H

#define LENGTH(X) (sizeof X / sizeof X[0])

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

#include "client.h"
#include "types.h"

#define BUTTON_EVENT_MASK XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE

void *emalloc(size_t size);

/*
 * registering events
 */
void xcb_register_key_events(Key key);

xcb_keycode_t *xcb_get_keycodes(xcb_keysym_t);
xcb_keysym_t xcb_get_keysym(xcb_keycode_t);

/*
 * Atoms
 */

#define WM_DELETE_WINDOW "WM_DELETE_WINDOW"
#define WM_PROTOCOLS "WM_PROTOCOLS"

xcb_atom_t xcb_get_atom(const char *);
bool xcb_send_atom(client *, xcb_atom_t);

#endif /* KBGWM_XCBUTILS_H */
