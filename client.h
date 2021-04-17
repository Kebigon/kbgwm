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

#ifndef KBGWM_CLIENT_H
#define KBGWM_CLIENT_H

#include "types.h"
#include <stdbool.h>

typedef struct client_t client;
struct client_t
{
    xcb_window_t id;
    int16_t x, y;
    uint16_t width, height;
    int32_t min_width, min_height;
    int32_t max_width, max_height;
    bool maximized;
    client *previous;
    client *next;
};

void client_grab_buttons(client *, bool);
void client_kill(const Arg *);
void client_create(xcb_window_t);
void client_toggle_maximize(const Arg *);
client *client_remove();
void client_add_workspace(client *, uint_fast8_t);
client *client_remove_workspace(uint_fast8_t);
client *client_find(xcb_window_t);
void client_maximize(client *);
void client_unmaximize(client *);
void client_sanitize_position(client *);
void client_sanitize_dimensions(client *);
void client_remove_all_workspaces(xcb_window_t);
client *client_find_all_workspaces(xcb_window_t);
client *client_find_workspace(xcb_window_t, uint_fast8_t);

#endif /* KBGWM_CLIENT_H */
