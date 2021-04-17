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

#include "events.h"
#include "client.h"
#include "kbgwm.h"
#include "xcbutils.h"

#include <assert.h>
#include <stdio.h>

#define CLEANMASK(mask) (mask & ~(numlockmask | XCB_MOD_MASK_LOCK))

#define EVENT_HANDLERS_SIZE XCB_CONFIGURE_REQUEST + 1
static void (*event_handlers[EVENT_HANDLERS_SIZE])(xcb_generic_event_t *);

static void handle_key_press(xcb_generic_event_t *e)
{
    xcb_key_press_event_t *event = (xcb_key_press_event_t *)e;
    xcb_keysym_t keysym = xcb_get_keysym(event->detail);

    for (uint_fast8_t i = 0; i < keys_length; i++)
    {
        if (keysym == keys[i].keysym && CLEANMASK(keys[i].modifiers) == CLEANMASK(event->state))
        {
            keys[i].func(&keys[i].arg);
            break;
        }
    }
}

static void handle_button_press(xcb_generic_event_t *e)
{
    xcb_button_press_event_t *event = (xcb_button_press_event_t *)e;

    // Click on the root window
    if (event->event == event->root && event->child == 0)
        return; // Nothing to be done

    xcb_window_t window = event->event == event->root ? event->child : event->event;

    // The window clicked is not the one in focus, we have to focus it
    if (workspaces[current_workspace] == NULL || window != workspaces[current_workspace]->id)
    {
        client *client = client_find(window);
        assert(client != NULL);

        focus_unfocus();
        workspaces[current_workspace] = client;
        focus_apply();
    }

    for (uint_fast8_t i = 0; i < buttons_length; i++)
    {
        if (event->detail == buttons[i].keysym &&
            CLEANMASK(buttons[i].modifiers) == CLEANMASK(event->state))
        {
            previous_x = event->root_x;
            previous_y = event->root_y;

            buttons[i].func(&buttons[i].arg);
            break;
        }
    }
}

static void handle_button_release(__attribute__((unused)) xcb_generic_event_t *event)
{
    // We were not moving or resizing the focused client
    if (!moving && !resizing)
        return; // Nothing to be done

    xcb_ungrab_pointer(c, XCB_CURRENT_TIME);
    xcb_flush(c);

    moving = false;
    resizing = false;
}

static void handle_motion_notify(xcb_generic_event_t *e)
{
    xcb_motion_notify_event_t *event = (xcb_motion_notify_event_t *)e;
    client *client = workspaces[current_workspace];

    assert(moving || resizing);
    assert(client != NULL);
    assert(client->id != root);

    if (client->maximized)
        client_unmaximize(client);

    int16_t diff_x = event->root_x - previous_x;
    int16_t diff_y = event->root_y - previous_y;
    previous_x = event->root_x;
    previous_y = event->root_y;

    if (moving)
    {
        client->x += diff_x;
        client->y += diff_y;
        client_sanitize_position(client);

        uint32_t values[2] = {client->x, client->y};
        xcb_configure_window(c, client->id, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values);
    }
    else if (resizing)
    {
        client->width += diff_x;
        client->height += diff_y;
        client_sanitize_dimensions(client);

        uint32_t values[2] = {client->width, client->height};
        xcb_configure_window(c, client->id, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                             values);
    }

    xcb_flush(c);
}

static void handle_destroy_notify(xcb_generic_event_t *e)
{
    xcb_destroy_notify_event_t *event = (xcb_destroy_notify_event_t *)e;

    client_remove_all_workspaces(event->window);
    if (focused_client != NULL)
        focus_apply();
}

static void handle_unmap_notify(xcb_generic_event_t *e)
{
    xcb_unmap_notify_event_t *event = (xcb_unmap_notify_event_t *)e;
    client *client = client_find_all_workspaces(event->window);

    // We don't know this client
    if (client == NULL)
        return; // Nothing to be done

    //  true if this came from a SendEvent request
    bool send_event = event->response_type & 0x80;
    if (!send_event)
        return; // Nothing to be done

    client_remove_all_workspaces(event->window);
    if (focused_client != NULL)
        focus_apply();
}

static void handle_map_request(xcb_generic_event_t *e)
{
    xcb_map_request_event_t *event = (xcb_map_request_event_t *)e;

    client_create(event->window);
}

static void handle_configure_request(xcb_generic_event_t *e)
{
    xcb_configure_request_event_t *event = (xcb_configure_request_event_t *)e;
    client *client = client_find_all_workspaces(event->window);

    if (client != NULL)
    {
        // The client is maximized
        if (client->maximized)
            return; // Nothing to be done

        if (event->value_mask & XCB_CONFIG_WINDOW_X)
            client->x = event->x;
        if (event->value_mask & XCB_CONFIG_WINDOW_Y)
            client->y = event->y;
        if (event->value_mask & XCB_CONFIG_WINDOW_WIDTH)
            client->width = event->width;
        if (event->value_mask & XCB_CONFIG_WINDOW_HEIGHT)
            client->height = event->height;

        // Ignored: XCB_CONFIG_WINDOW_BORDER_WIDTH, XCB_CONFIG_WINDOW_SIBLING,
        // XCB_CONFIG_WINDOW_STACK_MODE

        client_sanitize_position(client);
        client_sanitize_dimensions(client);

        uint32_t values[4] = {client->x, client->y, client->width, client->height};
        xcb_configure_window(c, client->id,
                             XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH |
                                 XCB_CONFIG_WINDOW_HEIGHT,
                             values);
        xcb_flush(c);
    }

    // We don't know the client -> apply the requested change
    else
    {
        uint16_t value_mask = 0;
        uint32_t value_list[7];
        int8_t i = 0;

        if (event->value_mask & XCB_CONFIG_WINDOW_X)
        {
            value_mask |= XCB_CONFIG_WINDOW_X;
            value_list[i++] = event->x;
        }
        if (event->value_mask & XCB_CONFIG_WINDOW_Y)
        {
            value_mask |= XCB_CONFIG_WINDOW_Y;
            value_list[i++] = event->y;
        }
        if (event->value_mask & XCB_CONFIG_WINDOW_WIDTH)
        {
            value_mask |= XCB_CONFIG_WINDOW_WIDTH;
            value_list[i++] = event->width;
        }
        if (event->value_mask & XCB_CONFIG_WINDOW_HEIGHT)
        {
            value_mask |= XCB_CONFIG_WINDOW_HEIGHT;
            value_list[i++] = event->height;
        }
        if (event->value_mask & XCB_CONFIG_WINDOW_SIBLING)
        {
            value_mask |= XCB_CONFIG_WINDOW_SIBLING;
            value_list[i++] = event->sibling;
        }
        if (event->value_mask & XCB_CONFIG_WINDOW_STACK_MODE)
        {
            value_mask |= XCB_CONFIG_WINDOW_STACK_MODE;
            value_list[i++] = event->stack_mode;
        }
        if (i != 0)
        {
            xcb_configure_window(c, event->window, value_mask, value_list);
            xcb_flush(c);
        }
    }
}

void handle_event(xcb_generic_event_t *event)
{
    void (*event_handler)(xcb_generic_event_t *) = event_handlers[event->response_type & ~0x80];
    if (event_handler == NULL)
        printf("Received unhandled event, response type %d\n", event->response_type & ~0x80);
    else
        event_handler(event);
}

void setup_events()
{
    /*
     * Initialize event_handlers
     */

    for (uint_fast8_t i = 0; i != EVENT_HANDLERS_SIZE; i++)
        event_handlers[i] = NULL;

    event_handlers[XCB_KEY_PRESS] = handle_key_press;
    event_handlers[XCB_BUTTON_PRESS] = handle_button_press;
    event_handlers[XCB_BUTTON_RELEASE] = handle_button_release;
    event_handlers[XCB_MOTION_NOTIFY] = handle_motion_notify;
    event_handlers[XCB_DESTROY_NOTIFY] = handle_destroy_notify;
    event_handlers[XCB_UNMAP_NOTIFY] = handle_unmap_notify;
    event_handlers[XCB_MAP_REQUEST] = handle_map_request;
    event_handlers[XCB_CONFIGURE_REQUEST] = handle_configure_request;

    /*
     * Register X11 events
     */

    uint32_t values[] = {XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_STRUCTURE_NOTIFY |
                         XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY};

    xcb_change_window_attributes_checked(c, root, XCB_CW_EVENT_MASK, values);

    for (uint_fast8_t i = 0; i != keys_length; i++)
        xcb_register_key_events(keys[i]);

    xcb_flush(c);
}
