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

#include <stdbool.h>
#include <stdint.h>
#include <xcb/xproto.h>

typedef union {
    const bool b;
    const uint_least8_t i;
    const char **cmd;
} Arg;

typedef struct
{
    uint16_t modifiers;
    xcb_keysym_t keysym;
    void (*func)(const Arg *);
    const Arg arg;
} Key;

typedef struct
{
    uint16_t modifiers;
    xcb_button_t keysym;
    void (*func)(const Arg *);
    const Arg arg;
} Button;
