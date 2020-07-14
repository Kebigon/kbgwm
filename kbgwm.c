#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>

#include <xcb/xcb.h>
#include <X11/keysym.h>
#include <inttypes.h>
#include "xcbutils.h"

#define IGNORE_LOCK(modifier) (modifier & ~(XCB_MOD_MASK_LOCK))
#define MATCH_MODIFIERS(expected, actual) ((actual & expected) == expected)

static void start(const Arg* arg);
static void mousemove(const Arg* arg);
static void mouseresize(const Arg* arg);

static void client_add(client*);
static void client_add_workspace(client*, uint_fast8_t);
static client* client_find(xcb_window_t);
static client* client_remove();
static client* client_remove_workspace(uint_fast8_t);
static void focus_apply(client*);
static void focus_next(const Arg*);
static void handle_button_press(xcb_button_press_event_t*);
static void handle_button_release(xcb_button_release_event_t*);
static void handle_key_press(xcb_key_press_event_t*);
static void handle_map_request(xcb_map_request_event_t*);
static void handle_mapping_notify(xcb_mapping_notify_event_t*);
static void handle_motion_notify(xcb_motion_notify_event_t*);
static void workspace_change(const Arg*);
static void workspace_next(const Arg*);
static void workspace_previous(const Arg*);
static void workspace_send(const Arg*);
static void workspace_set(uint_fast8_t);

#include "config.h"

#define BORDER_WIDTH_X2 (BORDER_WIDTH << 1)

bool quit = false;
bool moving = false;
bool resizing = false;
xcb_connection_t* c;
xcb_window_t root;
xcb_screen_t* screen;
uint_least16_t previous_x;
uint_least16_t previous_y;

#define focusedWindow workspaces[current_workspace]
uint_fast8_t current_workspace = 0;
static client* workspaces[NB_WORKSPACES];

void setupEvents()
{
	uint32_t values[] = { XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
		                  | XCB_EVENT_MASK_STRUCTURE_NOTIFY
		                  | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY };

	xcb_change_window_attributes_checked(c, root, XCB_CW_EVENT_MASK, values);

	xcb_grab_button(c, 1, root, XCB_EVENT_MASK_BUTTON_PRESS,
	                XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, root,
	                XCB_NONE, XCB_BUTTON_INDEX_1, 0 | XCB_MOD_MASK_2);
	xcb_grab_button(c, 1, root, XCB_EVENT_MASK_BUTTON_PRESS,
	                XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, root,
	                XCB_NONE, XCB_BUTTON_INDEX_3, 0 | XCB_MOD_MASK_2);

	for (uint_fast8_t i = 0; i != LENGTH(keys); i++)
		xcb_register_key_events(keys[i]);

	for (uint_fast8_t i = 0; i != LENGTH(buttons); i++)
		xcb_register_button_events(buttons[i]);

	xcb_flush(c);
}

void mousemove(__attribute__((unused)) const Arg* arg)
{
	printf("mousemove\n");
	moving = true;

	xcb_grab_pointer(c, 0, screen->root,
	                 XCB_EVENT_MASK_BUTTON_MOTION | XCB_EVENT_MASK_BUTTON_RELEASE,
	                 XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, screen->root,
	                 XCB_NONE,
	                 XCB_CURRENT_TIME);
	xcb_flush(c);
}

void mouseresize(__attribute__((unused)) const Arg* arg)
{
	printf("mouseresize\n");
	resizing = true;

	xcb_grab_pointer(c, 0, screen->root,
	                 XCB_EVENT_MASK_BUTTON_MOTION | XCB_EVENT_MASK_BUTTON_RELEASE,
	                 XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, screen->root,
	                 XCB_NONE,
	                 XCB_CURRENT_TIME);
	xcb_flush(c);
}

void eventLoop()
{
	while (!quit)
	{
		xcb_generic_event_t* event = xcb_wait_for_event(c);
		printf("event: received\n");

		switch (event->response_type & ~0x80)
		{
			case XCB_BUTTON_PRESS:
				handle_button_press((xcb_button_press_event_t*) event);
				break;

			case XCB_BUTTON_RELEASE:
				handle_button_release((xcb_button_release_event_t*) event);
				break;

			case XCB_KEY_PRESS:
				handle_key_press((xcb_key_press_event_t*) event);
				break;

			case XCB_MAP_REQUEST:
				handle_map_request((xcb_map_request_event_t*) event);
				break;

			case XCB_MAPPING_NOTIFY:
				handle_mapping_notify((xcb_mapping_notify_event_t*) event);
				break;

			case XCB_MOTION_NOTIFY:
				handle_motion_notify((xcb_motion_notify_event_t*) event);
				break;

			default:
				printf("Received event, response type %d\n",
				       event->response_type & ~0x80);
				break;
		}

		printf("event: handled\n");
		free(event);
		printf("event: freed\n");
		printf("\n");
	}
}

void start(const Arg* arg)
{
	printf("start %s\n", arg->cmd[0]);

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
	else {
		client->next = workspaces[workspace];
		client->previous = workspaces[workspace]->previous;
		client->next->previous = client;
		client->previous->next = client;
	}

	workspaces[workspace] = client;
}

// Find a client in the current workspace list
client* client_find(xcb_window_t id)
{
	client* client = workspaces[current_workspace];

	if (client != NULL)
		do
		{
			if (client->id == id)
				return client;
		}
		while ((client = client->next) != workspaces[current_workspace]);

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
	else {
		client->previous->next = client->next;
		client->next->previous = client->previous;
		workspaces[workspace] = client->next;
	}

	return client;
}

/*
 * Focus
 */

void focus_apply(client* new_focus)
{
	assert(workspaces[current_workspace] != NULL);
	assert(new_focus != NULL);

	printf("focus_apply: old=%d new=%d\n", workspaces[current_workspace]->id, new_focus->id);

	uint32_t values[1];

	// We change the color of the previously focused client
	xcb_change_window_attributes(c, workspaces[current_workspace]->id, XCB_CW_BORDER_PIXEL, (uint32_t[]) { 0xFF000000 | UNFOCUS_COLOR });

	// We change the color of the focused client
	xcb_change_window_attributes(c, new_focus->id, XCB_CW_BORDER_PIXEL, (uint32_t[]) { 0xFF000000 | FOCUS_COLOR });

	// Raise the window so it is on top
	values[0] = XCB_STACK_MODE_TOP_IF;
	xcb_configure_window(c, new_focus->id, XCB_CONFIG_WINDOW_STACK_MODE, values);

	// Set the keyboard on the focused window
	xcb_set_input_focus(c, XCB_NONE, new_focus->id, XCB_CURRENT_TIME);
	xcb_flush(c);

	printf("focus_apply: done\n");
}

// Focus the next client in the current workspace list
// arg->b : reverse mode
void focus_next(const Arg* arg)
{
	// No clients in the current workspace list
	// Only one client in the current workspace list
	if (workspaces[current_workspace] == NULL || workspaces[current_workspace]->next == workspaces[current_workspace])
		return; // Nothing to be done

	client* new_focus = arg->b ? workspaces[current_workspace]->previous : workspaces[current_workspace]->next;

	focus_apply(new_focus);

	// Move the newly focused window to the front of the list
	workspaces[current_workspace] = new_focus;
}

/*
 * Handling events from the event loop
 */

void handle_button_press(xcb_button_press_event_t* event)
{
	printf("handle_button_press: client=%d modifier=%d button=%d\n", event->child, event->state, event->detail);

	// Click on the root window
	if (event->child == 0)
		return; // Nothing to be done

	// The window clicked is not the one in focus, we have to focus it
	if (focusedWindow == NULL || event->child != focusedWindow->id)
		focus_apply(client_find(event->child));

	for (uint_fast8_t i = 0; i < LENGTH(buttons); i++)
	{
		if (event->detail == buttons[i].keysym && MATCH_MODIFIERS(buttons[i].modifiers,event->state))
		{
			previous_x = event->root_x;
			previous_y = event->root_y;

			buttons[i].func(&buttons[i].arg);
			break;
		}
	}

	printf("handle_button_press: done\n");
}

void handle_button_release(__attribute__((unused)) xcb_button_release_event_t* event)
{
	printf("XCB_BUTTON_RELEASE\n");

	assert(moving || resizing);

	xcb_ungrab_pointer(c, XCB_CURRENT_TIME);
	xcb_flush(c);

	moving = false;
	resizing = false;
}

void handle_key_press(xcb_key_press_event_t* event)
{
	printf("XCB_KEY_PRESS: detail=%d state=%d\n", event->detail, event->state);
	xcb_keysym_t keysym = xcb_get_keysym(event->detail);

	for (uint_fast8_t i = 0; i < LENGTH(keys); i++)
	{
		if (keysym == keys[i].keysym && MATCH_MODIFIERS(keys[i].modifiers, event->state))
		{
			keys[i].func(&keys[i].arg);
			break;
		}
	}
}

void handle_map_request(xcb_map_request_event_t* event)
{
	printf("handle_map_request: parent %d xcb_window_t %d\n", event->parent, event->window);

	xcb_get_geometry_reply_t* geometry = xcb_get_geometry_reply(c, xcb_get_geometry_unchecked(c, event->window), NULL);

	client* new_client = emalloc(sizeof(client));

	new_client->id = event->window;
	new_client->x = geometry->x;
	new_client->y = geometry->y;
	new_client->width = geometry->width;
	new_client->height = geometry->height;

	focus_apply(new_client);
	client_add(new_client);

	printf("new window: id=%d x=%d y=%d width=%d height=%d\n", new_client->id, new_client->x, new_client->y, new_client->width, new_client->height);

	xcb_configure_window(c, new_client->id, XCB_CONFIG_WINDOW_BORDER_WIDTH, (uint32_t[]) { BORDER_WIDTH });

	// Display the client
	xcb_map_window(c, new_client->id);
	xcb_flush(c);

	free(geometry);

	printf("handle_map_request: done\n");
}

void handle_mapping_notify(xcb_mapping_notify_event_t* event)
{
	printf("sequence %d\n", event->sequence);
	printf("request %d\n", event->request);
	printf("first_keycode %d\n", event->first_keycode);
	printf("count %d\n", event->count);
}

void handle_motion_notify(xcb_motion_notify_event_t* event)
{
	assert(moving || resizing);
	assert(focusedWindow != NULL);
	assert(focusedWindow->id != root);

	printf("handle_motionnotify: root_x=%d root_y=%d event_x=%d event_y=%d\n",
	       event->root_x, event->root_y, event->event_x, event->event_y);

	uint16_t diff_x = event->root_x - previous_x;
	uint16_t diff_y = event->root_y - previous_y;
	previous_x = event->root_x;
	previous_y = event->root_y;

	if (moving)
	{
		focusedWindow->x += diff_x;
		focusedWindow->y += diff_y;

		if (focusedWindow->x < 0)
			focusedWindow->x = 0;
		else if (focusedWindow->x + focusedWindow->width + BORDER_WIDTH_X2 > screen->width_in_pixels)
			focusedWindow->x = screen->width_in_pixels - focusedWindow->width - BORDER_WIDTH_X2;

		if (focusedWindow->y < 0)
			focusedWindow->y = 0;
		else if (focusedWindow->y + focusedWindow->height + BORDER_WIDTH_X2 > screen->height_in_pixels)
			focusedWindow->y = screen->height_in_pixels - focusedWindow->height - BORDER_WIDTH_X2;

		uint32_t values[2] = { focusedWindow->x, focusedWindow->y };
		xcb_configure_window(c, focusedWindow->id, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values);
	}
	else if (resizing)
	{
		focusedWindow->width += diff_x;
		focusedWindow->height += diff_y;

		uint32_t values[2] = { focusedWindow->width, focusedWindow->height };
		xcb_configure_window(c, focusedWindow->id, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
	}

	xcb_flush(c);
}

/*
 * Workspaces
 */

void workspace_change(const Arg* arg)
{
	printf("workspace_change: i=%d\n", arg->i);
	workspace_set(arg->i);
	printf("workspace_change: done\n");
}

void workspace_next(__attribute__((unused)) const Arg* arg)
{
	printf("workspace_next\n");
	workspace_set(current_workspace + 1 == NB_WORKSPACES ? 0 : current_workspace + 1 );
	printf("workspace_next: done\n");
}

void workspace_previous(__attribute__((unused)) const Arg* arg)
{
	printf("workspace_previous\n");
	workspace_set(current_workspace == 0 ? NB_WORKSPACES - 1 : current_workspace - 1);
	printf("workspace_previous: done\n");
}

void workspace_send(const Arg* arg)
{
	printf("workspace_send: i=%d\n", arg->i);
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
		do {
			xcb_unmap_window(c, client->id);
		}
		while ((client = client->next) != workspaces[current_workspace]);

	// Map the clients of the new workspace (if any)
	client = workspaces[new_workspace];
	if (client != NULL)
		do {
			xcb_map_window(c, client->id);
		}
		while ((client = client->next) != workspaces[new_workspace]);

	xcb_flush(c);
	current_workspace = new_workspace;

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
	c = xcb_connect(
		NULL, &screenp);

	if (xcb_connection_has_error(c))
	{
		perror("xcb_connect");
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

	setupEvents();

	// Event loop
	eventLoop();

	xcb_disconnect(c);

	return (0);
}
