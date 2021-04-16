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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>
#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <X11/keysym.h>
#include <inttypes.h>
#include "xcbutils.h"
#include "client.h"
#include "events.h"
#include "types.h"
#include <X11/keysym.h>

#include "kbgwm.h"
#include "config.h"

bool running = true;
bool moving = false;
bool resizing = false;
xcb_connection_t* c;
xcb_window_t root;
xcb_screen_t* screen;
uint_least16_t previous_x;
uint_least16_t previous_y;
uint16_t numlockmask = 0;
xcb_atom_t wm_protocols;
xcb_atom_t wm_delete_window;

uint_fast8_t current_workspace = 0;
client* workspaces[NB_WORKSPACES];

static inline void debug_print_globals()
{
	printf("current_workspace=%d\n", current_workspace);

	for (int workspace = 0; workspace != workspaces_length; workspace++)
	{
		if (workspaces[workspace] == NULL)
			printf("%d\tNULL\n", workspace);
		else
		{
			client* client = workspaces[workspace];
			do
			{
				printf("%d\tid=%d x=%d y=%d width=%d height=%d min_width=%d min_height=%d max_width=%d max_height=%d\n", workspace,
				       client->id, client->x, client->y, client->width, client->height, client->min_width, client->min_height,
				       client->max_width, client->max_height);
			} while ((client = client->next) != workspaces[workspace]);
		}
	}
}

static void debug_print_event(xcb_generic_event_t* event)
{
	switch (event->response_type & ~0x80)
	{
		case XCB_KEY_PRESS:
		{
			xcb_key_press_event_t* event2 = (xcb_key_press_event_t*) event;
			printf("=======[ event: XCB_KEY_PRESS ]=======\n");
			printf("keycode=%d modifiers=%d\n", event2->detail, event2->state);
			debug_print_globals();
			break;
		}
		case XCB_BUTTON_PRESS:
		{
			xcb_button_press_event_t* event2 = (xcb_button_press_event_t*) event;
			printf("=======[ event: XCB_BUTTON_PRESS ]=======\n");
			printf("window=%d child=%d modifiers=%d button=%d\n", event2->event, event2->child, event2->state, event2->detail);
			debug_print_globals();
			break;
		}
		case XCB_BUTTON_RELEASE:
		{
			printf("=======[ event: XCB_BUTTON_RELEASE ]=======\n");
			debug_print_globals();
			break;
		}
		case XCB_MOTION_NOTIFY:
		{
			xcb_motion_notify_event_t* event2 = (xcb_motion_notify_event_t*) event;
			printf("=======[ event: XCB_MOTION_NOTIFY ]=======\n");
			printf("root_x=%d root_y=%d event_x=%d event_y=%d\n", event2->root_x, event2->root_y, event2->event_x, event2->event_y);
			debug_print_globals();
			break;
		}
		case XCB_DESTROY_NOTIFY:
		{
			xcb_destroy_notify_event_t* event2 = (xcb_destroy_notify_event_t*) event;
			printf("=======[ event: XCB_DESTROY_NOTIFY ]=======\n");
			printf("window=%d\n", event2->window);
			debug_print_globals();
			break;
		}
		case XCB_UNMAP_NOTIFY:
		{
			xcb_unmap_notify_event_t* event2 = (xcb_unmap_notify_event_t*) event;
			printf("=======[ event: XCB_UNMAP_NOTIFY ]=======\n");
			printf("window=%d event=%d from_configure=%d send_event=%d\n", event2->window, event2->event, event2->from_configure,
			       event->response_type & 0x80);
			debug_print_globals();
			break;
		}
		case XCB_MAP_NOTIFY:
		{
			xcb_map_notify_event_t* event2 = (xcb_map_notify_event_t*) event;
			printf("=======[ event: XCB_MAP_NOTIFY ]=======\n");
			printf("window=%d\n", event2->window);
			debug_print_globals();
			break;
		}
		case XCB_MAP_REQUEST:
		{
			xcb_map_request_event_t* event2 = (xcb_map_request_event_t*) event;
			printf("=======[ event: XCB_MAP_REQUEST ]=======\n");
			printf("parent %d window %d\n", event2->parent, event2->window);
			debug_print_globals();
			break;
		}
		case XCB_CONFIGURE_REQUEST:
		{
			xcb_configure_request_event_t* event2 = (xcb_configure_request_event_t*) event;
			printf("=======[ event: XCB_CONFIGURE_REQUEST ]=======\n");
			printf("parent %d window %d\n", event2->parent, event2->window);
			debug_print_globals();
			break;
		}
		case XCB_MAPPING_NOTIFY:
		{
			xcb_mapping_notify_event_t* event2 = (xcb_mapping_notify_event_t*) event;
			printf("=======[ event: XCB_MAPPING_NOTIFY ]=======\n");
			printf("sequence %d\n", event2->sequence);
			printf("request %d\n", event2->request);
			printf("first_keycode %d\n", event2->first_keycode);
			printf("count %d\n", event2->count);
			debug_print_globals();
			break;
		}
		default:
		{
			printf("=======[ event: unlogged, response type %d ]=======\n", event->response_type & ~0x80);
			break;
		}
	}
}

void mousemove(__attribute__((unused))const Arg* arg)
{
	printf("=======[ user action: mousemove ]=======\n");
	moving = true;

	xcb_grab_pointer(c, 0, screen->root, XCB_EVENT_MASK_BUTTON_MOTION | XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
	                 XCB_GRAB_MODE_ASYNC, screen->root,
	                 XCB_NONE,
	                 XCB_CURRENT_TIME);
	xcb_flush(c);
}

void mouseresize(__attribute__((unused))const Arg* arg)
{
	printf("=======[ user action: mouseresize ]=======\n");
	resizing = true;

	xcb_grab_pointer(c, 0, screen->root, XCB_EVENT_MASK_BUTTON_MOTION | XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
	                 XCB_GRAB_MODE_ASYNC, screen->root,
	                 XCB_NONE,
	                 XCB_CURRENT_TIME);
	xcb_flush(c);
}

void eventLoop()
{
	while (running)
	{
		xcb_generic_event_t* event = xcb_wait_for_event(c);
		debug_print_event(event);

		handle_event(event);

		free(event);
		printf("=======[ event: DONE ]=======\n\n");
	}
}

void start(const Arg* arg)
{
	printf("=======[ user action: start ]=======\n");
	printf("cmd %s\n", arg->cmd[0]);

	if (fork() == 0)
	{
		// Child process
		setsid();

		if (execvp((char*) arg->cmd[0], (char**) arg->cmd) == -1)
		{
			perror(arg->cmd[0]);
			exit(-1);
		}
	}
}

/*
 * Focus
 */

void focus_apply()
{
	assert(workspaces[current_workspace] != NULL);

	// We change the color of the focused client
	xcb_change_window_attributes(c, workspaces[current_workspace]->id, XCB_CW_BORDER_PIXEL, (uint32_t[] )
	                             { 0xFF000000 | FOCUS_COLOR });

	// Raise the window so it is on top
	xcb_configure_window(c, workspaces[current_workspace]->id, XCB_CONFIG_WINDOW_STACK_MODE, (uint32_t[] )
	                     { XCB_STACK_MODE_ABOVE });

	// Set the keyboard on the focused window
	xcb_set_input_focus(c, XCB_INPUT_FOCUS_POINTER_ROOT, workspaces[current_workspace]->id, XCB_CURRENT_TIME);
	client_grab_buttons(workspaces[current_workspace], true);
	xcb_flush(c);

	printf("focus_apply: done\n");
}

// Focus the next client in the current workspace list
// arg->b : reverse mode
void focus_next(const Arg* arg)
{
	printf("=======[ user action: focus_next ]=======\n");

	// No clients in the current workspace list
	// Only one client in the current workspace list
	if (workspaces[current_workspace] == NULL || workspaces[current_workspace]->next == workspaces[current_workspace])
		return; // Nothing to be done

	client* new_focus = arg->b ? workspaces[current_workspace]->previous : workspaces[current_workspace]->next;

	focus_unfocus();
	workspaces[current_workspace] = new_focus;
	focus_apply();
}

// Remove the focus from the current client
void focus_unfocus()
{
	client* client = workspaces[current_workspace];

	// No client are focused
	if (client == NULL)
		return; // Nothing to be done

	// Change the border color to UNFOCUS_COLOR
	xcb_change_window_attributes(c, client->id, XCB_CW_BORDER_PIXEL, (uint32_t[] )
	                             { 0xFF000000 | UNFOCUS_COLOR });
	client_grab_buttons(workspaces[current_workspace], false);
}

/*
 * Handling events from the event loop
 */

void quit(__attribute__((unused))const Arg* arg)
{
	printf("=======[ user action: quit ]=======\n");
	running = false;
}

/*
 * Setup
 */

// Retrieve the numlock keycode
void setup_keyboard()
{
	xcb_get_modifier_mapping_reply_t* reply = xcb_get_modifier_mapping_reply(c, xcb_get_modifier_mapping_unchecked(c), NULL);
	if (!reply)
	{
		printf("Unable to retrieve midifier mapping");
		exit(-1);
	}

	xcb_keycode_t* modmap = xcb_get_modifier_mapping_keycodes(reply);
	if (!modmap)
	{
		printf("Unable to retrieve midifier mapping keycodes");
		exit(-1);
	}

	xcb_keycode_t* numlock = xcb_get_keycodes(XK_Num_Lock);

	// Not sure why 8, I looked at both dwm and 2bwm, and they both do the same
	for (uint_fast8_t i = 0; i < 8; i++)
	{
		for (uint_fast8_t j = 0; j < reply->keycodes_per_modifier; j++)
		{
			xcb_keycode_t keycode = modmap[i * reply->keycodes_per_modifier + j];

			if (keycode == *numlock)
			{
				numlockmask = (1 << i);
				printf("numlock is %d", keycode);
			}
		}
	}

	free(reply);
}

void setup_screen()
{
	// Retrieve the children of the root window
	xcb_query_tree_reply_t* reply = xcb_query_tree_reply(c, xcb_query_tree(c, screen->root), 0);
	if (NULL == reply)
	{
		printf("Unable to retrieve the root window's children");
		exit(-1);
	}

	int len = xcb_query_tree_children_length(reply);
	xcb_window_t* children = xcb_query_tree_children(reply);

	// Create the corresponding clients
	for (int i = 0; i != len; i++)
		client_create(children[i]);

	free(reply);
}

/*
 * Workspaces
 */

void workspace_change(const Arg* arg)
{
	printf("=======[ user action: focus_next ]=======\n");
	printf("i=%d\n", arg->i);

	workspace_set(arg->i);
}

void workspace_next(__attribute__((unused))const Arg* arg)
{
	printf("=======[ user action: workspace_next ]=======\n");

	workspace_set(current_workspace + 1 == workspaces_length ? 0 : current_workspace + 1);
}

void workspace_previous(__attribute__((unused))const Arg* arg)
{
	printf("=======[ user action: workspace_previous ]=======\n");
	workspace_set(current_workspace == 0 ? workspaces_length - 1 : current_workspace - 1);
}

void workspace_send(const Arg* arg)
{
	printf("=======[ user action: workspace_send ]=======\n");
	printf("i=%d\n", arg->i);
	debug_print_globals();

	uint_fast8_t new_workspace = arg->i;

	if (current_workspace == new_workspace || workspaces[current_workspace] == NULL)
		return; // Nothing to be done

	client* client = client_remove();
	client_add_workspace(client, new_workspace);

	xcb_unmap_window(c, client->id);
	xcb_flush(c);
	printf("workspace_send: done\n");
}

void workspace_set(uint_fast8_t new_workspace)
{
	printf("workspace_set: old=%d new=%d\n", current_workspace, new_workspace);

	if (current_workspace == new_workspace)
		return; // Nothing to be done

	// Unmap the clients of the current workspace (if any)
	client* client = workspaces[current_workspace];
	if (client != NULL)
		do
		{
			xcb_unmap_window(c, client->id);
		} while ((client = client->next) != workspaces[current_workspace]);

	// Map the clients of the new workspace (if any)
	client = workspaces[new_workspace];
	if (client != NULL)
		do
		{
			xcb_map_window(c, client->id);
		} while ((client = client->next) != workspaces[new_workspace]);

	xcb_flush(c);
	current_workspace = new_workspace;

	if (workspaces[current_workspace] != NULL)
		focus_apply();

	printf("workspace_set: done\n");
}

/*
 * Main
 */

int main(void)
{
	/*
	 * displayname = NULL -> use DISPLAY environment variable
	 */
	int screenp;
	c = xcb_connect(NULL, &screenp);

	if (xcb_connection_has_error(c))
	{
		printf("xcb_connect failed: %d", xcb_connection_has_error(c));
		exit(1);
	}

	/*
	 * Find the screen
	 */
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(c));

	for (int i = 0; i < screenp; ++i)
	{
		xcb_screen_next(&iter);
	}

	screen = iter.data;

	if (!screen)
	{
		xcb_disconnect(c);
		perror("screen not found");
		exit(1);
	}

	root = screen->root;

	xcb_flush(c);

	/*
	 * Initialize variables
	 */

	for (uint_fast8_t i = 0; i != workspaces_length; i++)
		workspaces[i] = NULL;

	wm_protocols = xcb_get_atom(WM_PROTOCOLS);
	wm_delete_window = xcb_get_atom(WM_DELETE_WINDOW);

	setup_keyboard();
	setup_screen();
	setup_events();

	// Event loop
	eventLoop();

	for (uint_fast8_t i = 0; i != workspaces_length; i++)
	{
		while (workspaces[i] != NULL)
		{
			client* client = client_remove_workspace(i);
			free(client);
		}
	}

	xcb_disconnect(c);

	return (0);
}
