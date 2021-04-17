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

#pragma once

#include "client.h"
#include "types.h"
#include <X11/keysym.h>

#define MODKEY XCB_MOD_MASK_1
#define SHIFT XCB_MOD_MASK_SHIFT

#define FOCUS_COLOR 0xFF0000
#define UNFOCUS_COLOR 0x005577

#define BORDER_WIDTH 1

/*
 * Number of workspaces
 * They will be numbered from 0 to NB_WORKSPACES-1
 */
#define NB_WORKSPACES 10

static const char *termcmd[] = {"xterm", NULL};
static const char *menucmd[] = {"dmenu_run", NULL};

// clang-format off
#define WORKSPACEKEYS(KEY,WORKSPACE) \
	{ MODKEY|XCB_MOD_MASK_SHIFT, KEY, workspace_send,   {.i = WORKSPACE} }, \
	{ MODKEY,                    KEY, workspace_change, {.i = WORKSPACE} },

const Key keys[] = {
	{ MODKEY,         XK_Return,    start,                  { .cmd = termcmd } },
	{ MODKEY,         XK_p,         start,                  { .cmd = menucmd } },
	{ MODKEY,         XK_Page_Up,   workspace_previous,     { 0 } },
	{ MODKEY,         XK_Page_Down, workspace_next,         { 0 } },
	{ MODKEY | SHIFT, XK_Tab,       focus_next,             { .b = true } },
	{ MODKEY,         XK_Tab,       focus_next,             { .b = false } },
	{ MODKEY,         XK_q,         client_kill,            { 0 } },
	{ MODKEY | SHIFT, XK_q,         quit,                   { 0 } },
	{ MODKEY,         XK_x,         client_toggle_maximize, { 0 } },
	WORKSPACEKEYS(XK_Home, 0)
	WORKSPACEKEYS(XK_1, 0)
	WORKSPACEKEYS(XK_2, 1)
	WORKSPACEKEYS(XK_3, 2)
	WORKSPACEKEYS(XK_4, 3)
	WORKSPACEKEYS(XK_5, 4)
	WORKSPACEKEYS(XK_6, 5)
	WORKSPACEKEYS(XK_7, 6)
	WORKSPACEKEYS(XK_8, 7)
	WORKSPACEKEYS(XK_9, 8)
	WORKSPACEKEYS(XK_0, 9)
	WORKSPACEKEYS(XK_End, 9)
};

const Button buttons[] = {
	{ MODKEY, XCB_BUTTON_INDEX_1, mousemove,   { 0 } },
	{ MODKEY, XCB_BUTTON_INDEX_3, mouseresize, { 0 } },
};
// clang-format on

const uint_least8_t keys_length = LENGTH(keys);
const uint_least8_t buttons_length = LENGTH(buttons);
const uint_least8_t workspaces_length = NB_WORKSPACES;
const uint_least8_t border_width = BORDER_WIDTH;
const uint_least8_t border_width_x2 = (border_width << 1);
