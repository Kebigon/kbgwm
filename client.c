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

#include "client.h"
#include "kbgwm.h"
#include "xcbutils.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb_icccm.h>

static inline int16_t int16_in_range(int16_t value, int16_t min, int16_t max)
{
    if (value < min)
        return min;
    else if (value > max)
        return max;
    else
        return value;
}

static inline uint16_t uint16_in_range(uint16_t value, uint16_t min, uint16_t max)
{
    if (value < min)
        return min;
    else if (value > max)
        return max;
    else
        return value;
}

// Add a client to the current workspace list
void client_add(client *client)
{
    client_add_workspace(client, current_workspace);
}

// Add a client to a workspace list
void client_add_workspace(client *client, uint_fast8_t workspace)
{
    assert(client != NULL);
    assert(workspace < workspaces_length);

    if (workspaces[workspace] == NULL)
    {
        client->next = client;
        client->previous = client;
    }
    else
    {
        client->next = workspaces[workspace];
        client->previous = workspaces[workspace]->previous;
        client->next->previous = client;
        client->previous->next = client;
    }

    workspaces[workspace] = client;
}

void client_create(xcb_window_t id)
{
    printf("client_create: id=%d\n", id);

    // Request the information for the window

    xcb_get_geometry_reply_t *geometry =
        xcb_get_geometry_reply(c, xcb_get_geometry_unchecked(c, id), NULL);
    xcb_size_hints_t hints;
    xcb_icccm_get_wm_normal_hints_reply(c, xcb_icccm_get_wm_normal_hints_unchecked(c, id), &hints,
                                        NULL);

    client *new_client = emalloc(sizeof(client));

    new_client->id = id;
    new_client->x = geometry->x;
    new_client->y = geometry->y;
    new_client->width = geometry->width;
    new_client->height = geometry->height;
    new_client->min_width = hints.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE ? hints.min_width : 0;
    new_client->min_height = hints.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE ? hints.min_height : 0;
    new_client->max_width = hints.flags & XCB_ICCCM_SIZE_HINT_P_MAX_SIZE ? hints.max_width : 0xFFFF;
    new_client->max_height =
        hints.flags & XCB_ICCCM_SIZE_HINT_P_MAX_SIZE ? hints.max_height : 0xFFFF;
    new_client->maximized = false;

    client_sanitize_dimensions(new_client);

    printf("new window: id=%d x=%d y=%d width=%d height=%d min_width=%d min_height=%d max_width=%d "
           "max_height=%d\n",
           id, new_client->x, new_client->y, new_client->width, new_client->height,
           new_client->min_width, new_client->min_height, new_client->max_width,
           new_client->max_height);

    xcb_configure_window(
        c, id, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_BORDER_WIDTH,
        (uint32_t[]){new_client->width, new_client->height, border_width});

    // Display the client
    xcb_map_window(c, new_client->id);
    xcb_flush(c);

    free(geometry);

    focus_unfocus();
    client_add(new_client);
    focus_apply();

    printf("client_create: done\n");
}

// Find a client in the current workspace list
client *client_find(xcb_window_t id)
{
    return client_find_workspace(id, current_workspace);
}

client *client_find_all_workspaces(xcb_window_t id)
{
    for (uint_fast8_t workspace = 0; workspace != workspaces_length; workspace++)
    {
        client *client = client_find_workspace(id, workspace);
        if (client != NULL)
            return client;
    }

    return NULL;
}

client *client_find_workspace(xcb_window_t id, uint_fast8_t workspace)
{
    assert(workspace < workspaces_length);

    client *client = workspaces[workspace];

    if (client != NULL)
        do
        {
            if (client->id == id)
                return client;
        } while ((client = client->next) != workspaces[workspace]);

    return NULL;
}

// Remove the focused client from the current workspace list
client *client_remove()
{
    return client_remove_workspace(current_workspace);
}

// Remove the focused client from a workspace list
client *client_remove_workspace(uint_fast8_t workspace)
{
    assert(workspace < workspaces_length);
    assert(workspaces[workspace] != NULL);

    client *client = workspaces[workspace];
    if (client->next == client)
        workspaces[workspace] = NULL;
    else
    {
        client->previous->next = client->next;
        client->next->previous = client->previous;
        workspaces[workspace] = client->next;
    }

    return client;
}

void client_remove_all_workspaces(xcb_window_t id)
{
    for (uint_fast8_t workspace = 0; workspace != workspaces_length; workspace++)
    {
        client *client = client_find_workspace(id, workspace);
        if (client != NULL)
        {
            if (client->next == client)
                workspaces[workspace] = NULL;
            else
            {
                client->previous->next = client->next;
                client->next->previous = client->previous;

                if (workspaces[workspace] == client)
                    workspaces[workspace] = client->next;
            }
        }
    }
}

void client_sanitize_position(client *client)
{
    int16_t x =
        int16_in_range(client->x, 0, screen->width_in_pixels - client->width - border_width_x2);
    if (client->x != x)
        client->x = x;

    int16_t y =
        int16_in_range(client->y, 0, screen->height_in_pixels - client->height - border_width_x2);
    if (client->y != y)
        client->y = y;
}

void client_sanitize_dimensions(client *client)
{
    uint16_t width = uint16_in_range(client->width, client->min_width, client->max_width);
    width = uint16_in_range(width, 0, screen->width_in_pixels - client->x - border_width_x2);
    if (client->width != width)
        client->width = width;

    uint16_t height = uint16_in_range(client->height, client->min_height, client->max_height);
    height = uint16_in_range(height, 0, screen->height_in_pixels - client->y - border_width_x2);
    if (client->height != height)
        client->height = height;
}

void client_kill(__attribute__((unused)) const Arg *arg)
{
    printf("=======[ user action: client_kill ]=======\n");

    // No client are focused
    if (workspaces[current_workspace] == NULL)
        return; // Nothing to be done

    if (!xcb_send_atom(workspaces[current_workspace], wm_delete_window))
    {
        // The client does not support WM_DELETE, let's kill it
        xcb_kill_client(c, workspaces[current_workspace]->id);
    }

    xcb_flush(c);
}

void client_toggle_maximize(__attribute__((unused)) const Arg *arg)
{
    printf("=======[ user action: client_toggle_maximize ]=======\n");

    // No client are focused
    if (workspaces[current_workspace] == NULL)
        return; // Nothing to be done

    client *client = workspaces[current_workspace];

    if (client->maximized)
        client_unmaximize(client);
    else
        client_maximize(client);

    xcb_flush(c);
}

void client_maximize(client *client)
{
    assert(client != NULL);
    assert(!client->maximized);

    client->maximized = true;

    uint32_t values[] = {0, 0, screen->width_in_pixels, screen->height_in_pixels, 0};
    xcb_configure_window(c, focused_client->id,
                         XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH |
                             XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_BORDER_WIDTH,
                         values);
}

void client_unmaximize(client *client)
{
    assert(client != NULL);
    assert(client->maximized);

    client->maximized = false;

    uint32_t values[] = {client->x, client->y, client->width, client->height, border_width};
    xcb_configure_window(c, focused_client->id,
                         XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH |
                             XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_BORDER_WIDTH,
                         values);
}

void client_grab_buttons(client *client, bool focused)
{
    uint16_t modifiers[] = {0, numlockmask, XCB_MOD_MASK_LOCK, numlockmask | XCB_MOD_MASK_LOCK};

    xcb_ungrab_button(c, XCB_BUTTON_INDEX_ANY, client->id, XCB_MOD_MASK_ANY);

    // The client is not the focused one -> grab everything
    if (!focused)
    {
        xcb_grab_button(c, 1, client->id, BUTTON_EVENT_MASK, XCB_GRAB_MODE_ASYNC,
                        XCB_GRAB_MODE_ASYNC, root, XCB_NONE, XCB_BUTTON_INDEX_ANY,
                        XCB_MOD_MASK_ANY);
    }

    // The client is the focused one -> grab only the configured buttons
    else
    {
        for (unsigned int i = 0; i != buttons_length; i++)
        {
            Button button = buttons[i];

            for (unsigned int j = 0; j != LENGTH(modifiers); j++)
            {
                xcb_grab_button(c, 0, client->id, BUTTON_EVENT_MASK, XCB_GRAB_MODE_ASYNC,
                                XCB_GRAB_MODE_ASYNC, root, XCB_NONE, button.keysym,
                                button.modifiers | modifiers[j]);
            }
        }
    }
}
