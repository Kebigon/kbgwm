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

#define CLEANMASK(mask) (mask & ~(numlockmask|XCB_MOD_MASK_LOCK))

static void start(const Arg* arg);
static void mousemove(const Arg* arg);
static void mouseresize(const Arg* arg);

static void client_add(client*);
static void client_add_workspace(client*, uint_fast8_t);
static client* client_find(xcb_window_t);
static client* client_find_workspace(xcb_window_t, uint_fast8_t);
static client* client_remove();
static client* client_remove_workspace(uint_fast8_t);
static void client_kill(const Arg*);
static void client_create(xcb_window_t);
static void client_sanitize_position(client*);
static void client_sanitize_dimensions(client*);
static void client_toggle_maximize(const Arg*);
static void client_maximize(client*);
static void client_unmaximize(client*);
static void focus_apply();
static void focus_next(const Arg*);
static void focus_unfocus();
static void handle_button_press(xcb_button_press_event_t*);
static void handle_button_release(xcb_button_release_event_t*);
static void handle_configure_request(xcb_configure_request_event_t*);
static void handle_destroy_notify(xcb_destroy_notify_event_t*);
static void handle_key_press(xcb_key_press_event_t*);
static void handle_map_request(xcb_map_request_event_t*);
static void handle_motion_notify(xcb_motion_notify_event_t*);
static void handle_unmap_notify(xcb_unmap_notify_event_t*);
static void quit(const Arg*);
static void workspace_change(const Arg*);
static void workspace_next(const Arg*);
static void workspace_previous(const Arg*);
static void workspace_send(const Arg*);
static void workspace_set(uint_fast8_t);

static inline int16_t int16_in_range(int16_t, int16_t, int16_t);
static inline uint16_t uint16_in_range(uint16_t, uint16_t, uint16_t);
static inline void debug_print_event();
static inline void debug_print_globals();

#include "config.h"

#define BORDER_WIDTH_X2 (BORDER_WIDTH << 1)

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

#define focused_client workspaces[current_workspace]
uint_fast8_t current_workspace = 0;
static client* workspaces[NB_WORKSPACES];

inline int16_t int16_in_range(int16_t value, int16_t min, int16_t max)
{
	if (value < min)
		return min;
	else if (value > max)
		return max;
	else
		return value;
}
inline uint16_t uint16_in_range(uint16_t value, uint16_t min, uint16_t max)
{
	if (value < min)
		return min;
	else if (value > max)
		return max;
	else
		return value;
}

void debug_print_event(xcb_generic_event_t* event)
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

inline void debug_print_globals()
{
	printf("current_workspace=%d\n", current_workspace);

	for (int workspace = 0; workspace != NB_WORKSPACES; workspace++)
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

		switch (event->response_type & ~0x80)
		{
			case XCB_KEY_PRESS:
				handle_key_press((xcb_key_press_event_t*) event);
				break;

			case XCB_BUTTON_PRESS:
				handle_button_press((xcb_button_press_event_t*) event);
				break;

			case XCB_BUTTON_RELEASE:
				handle_button_release((xcb_button_release_event_t*) event);
				break;

			case XCB_MOTION_NOTIFY:
				handle_motion_notify((xcb_motion_notify_event_t*) event);
				break;

			case XCB_DESTROY_NOTIFY:
				handle_destroy_notify((xcb_destroy_notify_event_t*) event);
				break;

			case XCB_UNMAP_NOTIFY:
				handle_unmap_notify((xcb_unmap_notify_event_t*) event);
				break;

			case XCB_MAP_REQUEST:
				handle_map_request((xcb_map_request_event_t*) event);
				break;

			case XCB_CONFIGURE_REQUEST:
				handle_configure_request((xcb_configure_request_event_t*) event);
				break;

			default:
				printf("Received unhandled event, response type %d\n", event->response_type & ~0x80);
				break;
		}

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
 * Clients
 */

// Add a client to the current workspace list
void client_add(client* client)
{
	client_add_workspace(client, current_workspace);
}

// Add a client to a workspace list
void client_add_workspace(client* client, uint_fast8_t workspace)
{
	assert(client != NULL);
	assert(workspace < NB_WORKSPACES);

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

	xcb_get_geometry_reply_t* geometry = xcb_get_geometry_reply(c, xcb_get_geometry_unchecked(c, id), NULL);
	xcb_size_hints_t hints;
	xcb_icccm_get_wm_normal_hints_reply(c, xcb_icccm_get_wm_normal_hints_unchecked(c, id), &hints,
	                                    NULL);

	client* new_client = emalloc(sizeof(client));

	new_client->id = id;
	new_client->x = geometry->x;
	new_client->y = geometry->y;
	new_client->width = geometry->width;
	new_client->height = geometry->height;
	new_client->min_width = hints.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE ? hints.min_width : 0;
	new_client->min_height = hints.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE ? hints.min_height : 0;
	new_client->max_width = hints.flags & XCB_ICCCM_SIZE_HINT_P_MAX_SIZE ? hints.max_width : 0xFFFF;
	new_client->max_height = hints.flags & XCB_ICCCM_SIZE_HINT_P_MAX_SIZE ? hints.max_height : 0xFFFF;
	new_client->maximized = false;

	client_sanitize_dimensions(new_client);

	printf("new window: id=%d x=%d y=%d width=%d height=%d min_width=%d min_height=%d max_width=%d max_height=%d\n", id,
	       new_client->x, new_client->y, new_client->width, new_client->height, new_client->min_width, new_client->min_height,
	       new_client->max_width, new_client->max_height);

	xcb_configure_window(c, id, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_BORDER_WIDTH, (uint32_t[] )
	                     { new_client->width, new_client->height, BORDER_WIDTH });

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
client* client_find(xcb_window_t id)
{
	return client_find_workspace(id, current_workspace);
}

client* client_find_all_workspaces(xcb_window_t id)
{
	for (uint_fast8_t workspace = 0; workspace != NB_WORKSPACES; workspace++)
	{
		client* client = client_find_workspace(id, workspace);
		if (client != NULL)
			return client;
	}

	return NULL;
}

client* client_find_workspace(xcb_window_t id, uint_fast8_t workspace)
{
	assert(workspace < NB_WORKSPACES);

	client* client = workspaces[workspace];

	if (client != NULL)
		do
		{
			if (client->id == id)
				return client;
		} while ((client = client->next) != workspaces[workspace]);

	return NULL;
}

// Remove the focused client from the current workspace list
client* client_remove()
{
	return client_remove_workspace(current_workspace);
}

// Remove the focused client from a workspace list
client* client_remove_workspace(uint_fast8_t workspace)
{
	assert(workspace < NB_WORKSPACES);
	assert(workspaces[workspace] != NULL);

	client* client = workspaces[workspace];
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
	for (uint_fast8_t workspace = 0; workspace != NB_WORKSPACES; workspace++)
	{
		client* client = client_find_workspace(id, workspace);
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

void client_sanitize_position(client* client)
{
	int16_t x = int16_in_range(client->x, 0, screen->width_in_pixels - client->width - BORDER_WIDTH_X2);
	if (client->x != x)
		client->x = x;

	int16_t y = int16_in_range(client->y, 0, screen->height_in_pixels - client->height - BORDER_WIDTH_X2);
	if (client->y != y)
		client->y = y;
}

void client_sanitize_dimensions(client* client)
{
	uint16_t width = uint16_in_range(client->width, client->min_width, client->max_width);
	width = uint16_in_range(width, 0, screen->width_in_pixels - client->x - BORDER_WIDTH_X2);
	if (client->width != width)
		client->width = width;

	uint16_t height = uint16_in_range(client->height, client->min_height, client->max_height);
	height = uint16_in_range(height, 0, screen->height_in_pixels - client->y - BORDER_WIDTH_X2);
	if (client->height != height)
		client->height = height;
}

void client_kill(__attribute__((unused))const Arg* arg)
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

void client_toggle_maximize(__attribute__((unused))const Arg* arg)
{
	printf("=======[ user action: client_toggle_maximize ]=======\n");

	// No client are focused
	if (workspaces[current_workspace] == NULL)
		return; // Nothing to be done

	client* client = workspaces[current_workspace];

	if (client->maximized)
		client_unmaximize(client);
	else
		client_maximize(client);

	xcb_flush(c);
}

void client_maximize(client* client)
{
	assert(client != NULL);
	assert(!client->maximized);

	client->maximized = true;

	uint32_t values[] = { 0, 0, screen->width_in_pixels, screen->height_in_pixels, 0 };
	xcb_configure_window(c, focused_client->id, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_BORDER_WIDTH, values);
}

void client_unmaximize(client* client)
{
	assert(client != NULL);
	assert(client->maximized);

	client->maximized = false;

	uint32_t values[] = { client->x, client->y, client->width, client->height, BORDER_WIDTH };
	xcb_configure_window(c, focused_client->id, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_BORDER_WIDTH, values);
}

void client_grab_buttons(client* client, bool focused)
{
	uint16_t modifiers[] = { 0, numlockmask, XCB_MOD_MASK_LOCK, numlockmask | XCB_MOD_MASK_LOCK };

	xcb_ungrab_button(c, XCB_BUTTON_INDEX_ANY, client->id, XCB_MOD_MASK_ANY);

	// The client is not the focused one -> grab everything
	if (!focused)
	{
		xcb_grab_button(c, 1, client->id, BUTTON_EVENT_MASK, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, root, XCB_NONE,
		                XCB_BUTTON_INDEX_ANY, XCB_MOD_MASK_ANY);
	}

	// The client is the focused one -> grab only the configured buttons
	else
	{
		for (unsigned int i = 0; i != LENGTH(buttons); i++)
		{
			Button button = buttons[i];

			for (unsigned int j = 0; j != LENGTH(modifiers); j++)
			{
				xcb_grab_button(c, 0, client->id, BUTTON_EVENT_MASK, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, root, XCB_NONE,
				                button.keysym, button.modifiers | modifiers[j]);
			}
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
	focus_apply(new_focus);
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

void handle_button_press(xcb_button_press_event_t* event)
{
	// Click on the root window
	if (event->event == event->root && event->child == 0)
		return; // Nothing to be done

	xcb_window_t window = event->event == event->root ? event->child : event->event;

	// The window clicked is not the one in focus, we have to focus it
	if (workspaces[current_workspace] == NULL || window != workspaces[current_workspace]->id)
	{
		client* client = client_find(window);
		assert(client != NULL);

		focus_unfocus();
		workspaces[current_workspace] = client;
		focus_apply();
	}

	for (uint_fast8_t i = 0; i < LENGTH(buttons); i++)
	{
		if (event->detail == buttons[i].keysym && CLEANMASK(buttons[i].modifiers) == CLEANMASK(event->state))
		{
			previous_x = event->root_x;
			previous_y = event->root_y;

			buttons[i].func(&buttons[i].arg);
			break;
		}
	}
}

void handle_button_release(__attribute__((unused)) xcb_button_release_event_t* event)
{
	// We were not moving or resizing the focused client
	if (!moving && !resizing)
		return; // Nothing to be done

	xcb_ungrab_pointer(c, XCB_CURRENT_TIME);
	xcb_flush(c);

	moving = false;
	resizing = false;
}

void handle_configure_request(xcb_configure_request_event_t* event)
{
	client* client = client_find_all_workspaces(event->window);

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

		// Ignored: XCB_CONFIG_WINDOW_BORDER_WIDTH, XCB_CONFIG_WINDOW_SIBLING, XCB_CONFIG_WINDOW_STACK_MODE

		client_sanitize_position(client);
		client_sanitize_dimensions(client);

		uint32_t values[4] = { client->x, client->y, client->width, client->height };
		xcb_configure_window(c, client->id,
		                     XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
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

void handle_destroy_notify(xcb_destroy_notify_event_t* event)
{
	client_remove_all_workspaces(event->window);
	if (focused_client != NULL)
		focus_apply();
}

void handle_key_press(xcb_key_press_event_t* event)
{
	xcb_keysym_t keysym = xcb_get_keysym(event->detail);

	for (uint_fast8_t i = 0; i < LENGTH(keys); i++)
	{
		if (keysym == keys[i].keysym && CLEANMASK(keys[i].modifiers) == CLEANMASK(event->state))
		{
			keys[i].func(&keys[i].arg);
			break;
		}
	}
}

void handle_map_request(xcb_map_request_event_t* event)
{
	client_create(event->window);
}

void handle_unmap_notify(xcb_unmap_notify_event_t* event)
{
	client* client = client_find_all_workspaces(event->window);

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

void handle_motion_notify(xcb_motion_notify_event_t* event)
{
	client* client = workspaces[current_workspace];

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

		uint32_t values[2] = { client->x, client->y };
		xcb_configure_window(c, client->id, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values);
	}
	else if (resizing)
	{
		client->width += diff_x;
		client->height += diff_y;
		client_sanitize_dimensions(client);

		uint32_t values[2] = { client->width, client->height };
		xcb_configure_window(c, client->id, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
	}

	xcb_flush(c);
}

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

void setup_events()
{
	uint32_t values[] = { XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_STRUCTURE_NOTIFY
		                  | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY };

	xcb_change_window_attributes_checked(c, root, XCB_CW_EVENT_MASK, values);

	for (uint_fast8_t i = 0; i != LENGTH(keys); i++)
		xcb_register_key_events(keys[i]);

	xcb_flush(c);
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

	workspace_set(current_workspace + 1 == NB_WORKSPACES ? 0 : current_workspace + 1);
}

void workspace_previous(__attribute__((unused))const Arg* arg)
{
	printf("=======[ user action: workspace_previous ]=======\n");
	workspace_set(current_workspace == 0 ? NB_WORKSPACES - 1 : current_workspace - 1);
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

	for (uint_fast8_t i = 0; i != NB_WORKSPACES; i++)
		workspaces[i] = NULL;

	wm_protocols = xcb_get_atom(WM_PROTOCOLS);
	wm_delete_window = xcb_get_atom(WM_DELETE_WINDOW);

	setup_keyboard();
	setup_screen();
	setup_events();

	// Event loop
	eventLoop();

	for (uint_fast8_t i = 0; i != NB_WORKSPACES; i++)
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
