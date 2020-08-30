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

#ifndef KBGWM_KBGWM_H
#define KBGWM_KBGWM_H

void start(const Arg* arg);
void mousemove(const Arg* arg);
void mouseresize(const Arg* arg);

void focus_apply();
void focus_next(const Arg*);
void focus_unfocus();
void quit(const Arg*);
void workspace_change(const Arg*);
void workspace_next(const Arg*);
void workspace_previous(const Arg*);
void workspace_send(const Arg*);
void workspace_set(uint_fast8_t);

#include <stdbool.h>
#include "types.h"

#define focused_client workspaces[current_workspace]

extern bool running;
extern bool moving;
extern bool resizing;
extern xcb_connection_t* c;
extern xcb_window_t root;
extern xcb_screen_t* screen;
extern uint_least16_t previous_x;
extern uint_least16_t previous_y;
extern uint16_t numlockmask;
extern xcb_atom_t wm_protocols;
extern xcb_atom_t wm_delete_window;
extern uint_fast8_t current_workspace;
extern client* workspaces[];

extern const Key keys[];
extern const Button buttons[];

extern const uint_least8_t keys_length;
extern const uint_least8_t buttons_length;
extern const uint_least8_t workspaces_length;
extern const uint_least8_t border_width;
extern const uint_least8_t border_width_x2;

#endif /* KBGWM_KBGWM_H */
