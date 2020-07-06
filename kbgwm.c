#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>

#include <xcb/xcb.h>
#include <X11/keysym.h>

#include "xcbutils.h"

#define IGNORE_LOCK(modifier) (modifier & ~(XCB_MOD_MASK_LOCK))
#define MATCH_MODIFIERS(expected, actual) ((actual & expected) == expected)

static void start(const Arg* arg);
static void mousemove(const Arg* arg);
static void mouseresize(const Arg* arg);

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

static window* focusedWindow;
window* windows;

window* findWindow(xcb_window_t id)
{
	window* window = windows;

	do
	{
		if (window->id == id)
			return (window);
	}
	while ((window = window->next) != NULL);

	return NULL;
}

void focus(xcb_window_t id)
{
	printf("focus: id=%d\n", id);

	uint32_t values[1];

	window* window = findWindow(id);
	assert(window != NULL);

	// A window was previously focused -> we change its color
	if (focusedWindow != NULL)
	{
		xcb_change_window_attributes(c, focusedWindow->id, XCB_CW_BORDER_PIXEL, (uint32_t[]) { 0xFF000000 | UNFOCUS_COLOR });
	}

	xcb_change_window_attributes(c, id, XCB_CW_BORDER_PIXEL, (uint32_t[]) { 0xFF000000 | FOCUS_COLOR });

	// Raise the window so it is on top
	values[0] = XCB_STACK_MODE_TOP_IF;
	xcb_configure_window(c, id, XCB_CONFIG_WINDOW_STACK_MODE, values);

	// Set the keyboard on the focused window
	xcb_set_input_focus(c, XCB_NONE, id, XCB_CURRENT_TIME);
	xcb_flush(c);

	focusedWindow = window;
	printf("focus set to: id=%d\n", focusedWindow->id);
}

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

void onMappingNotify(xcb_mapping_notify_event_t* event)
{
	printf("sequence %d\n", event->sequence);
	printf("request %d\n", event->request);
	printf("first_keycode %d\n", event->first_keycode);
	printf("count %d\n", event->count);
}

void handle_keypress(xcb_key_press_event_t* event)
{
	xcb_keysym_t keysym = xcb_get_keysym(event->detail);

	printf("received key: mod %d key %d\n", event->state, keysym);

	for (uint_fast8_t i = 0; i < LENGTH(keys); i++)
	{
		printf("testing key: mod %d key %d\n", keys[i].modifiers,
		       keys[i].keysym);

		if (keysym == keys[i].keysym && MATCH_MODIFIERS(keys[i].modifiers, event->state))
		{
			printf("Key found !\n");
			keys[i].func(&keys[i].arg);
			break;
		}
	}
}

void handle_maprequest(xcb_map_request_event_t* event)
{
	xcb_get_geometry_reply_t* geometry;

	printf("received map request: parent %d xcb_window_t %d\n", event->parent, event->window);

	geometry = xcb_get_geometry_reply(c, xcb_get_geometry(c, event->window), NULL);

	window* window = emalloc(sizeof window);
	window->id = event->window;
	window->x = geometry->x;
	window->y = geometry->y;
	window->width = geometry->width;
	window->height = geometry->height;

	if (windows != NULL)
	{
		window->next = windows;
	}

	windows = window;

	printf("new window: id=%d x=%d y=%d width=%d height=%d\n", window->id, window->x, window->y, window->width, window->height);

	focus(event->window);

	xcb_configure_window(c, window->id, XCB_CONFIG_WINDOW_BORDER_WIDTH, (uint32_t[]) { BORDER_WIDTH });

	// Display the window
	xcb_map_window(c, window->id);
	xcb_flush(c);
}

void handle_buttonpress(xcb_button_press_event_t* event)
{
	printf("handle_buttonpress: child=%d\n", event->child);

	// Click on the root window -> ignore
	if (event->child == 0)
		return;

	// Focus window if needed
	if (event->child != focusedWindow->id)
		focus(event->child);

	for (uint_fast8_t i = 0; i < LENGTH(buttons); i++)
	{
		if (event->detail == buttons[i].keysym && MATCH_MODIFIERS(buttons[i].modifiers,event->state))
		{
			previous_x = event->root_x;
			previous_y = event->root_y;

			printf("Button found !\n");
			buttons[i].func(&buttons[i].arg);
			break;
		}
	}
}

void handle_buttonrelease(xcb_button_release_event_t* event)
{
	assert(moving || resizing);

	printf("handle_buttonrelease: mod=%d button=%d\n", event->state, event->detail);

	xcb_ungrab_pointer(c, XCB_CURRENT_TIME);
	xcb_flush(c);

	moving = false;
	resizing = false;
}

void mousemove(const Arg* arg)
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

void mouseresize(const Arg* arg)
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

void handle_motionnotify(xcb_motion_notify_event_t* event)
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

void eventLoop()
{
	xcb_generic_event_t* event;

	while (!quit)
	{
		event = xcb_wait_for_event(c);

		switch (event->response_type & ~0x80)
		{
			case XCB_KEY_PRESS:
				printf("\nXCB_KEY_PRESS\n");
				handle_keypress((xcb_key_press_event_t*) event);
				break;

			case XCB_BUTTON_PRESS:
				printf("\nXCB_BUTTON_PRESS\n");
				handle_buttonpress((xcb_button_press_event_t*) event);
				break;

			case XCB_BUTTON_RELEASE:
				printf("\nXCB_BUTTON_RELEASE\n");
				handle_buttonrelease((xcb_button_release_event_t*) event);
				break;

			case XCB_MAP_REQUEST:
				printf("\nXCB_MAP_REQUEST\n");
				handle_maprequest((xcb_map_request_event_t*) event);
				break;

			case XCB_MAPPING_NOTIFY:
				printf("\nXCB_MAPPING_NOTIFY\n");
				onMappingNotify((xcb_mapping_notify_event_t*) event);
				break;

			case XCB_MOTION_NOTIFY:
				printf("\nXCB_MOTION_NOTIFY\n");
				handle_motionnotify((xcb_motion_notify_event_t*) event);
				break;

			default:
				printf("Received event, response type %d\n",
				       event->response_type & ~0x80);
				break;
		}

		free(event);
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

	printf("White: %d\n", screen->white_pixel);
	printf("Black: %d\n", screen->black_pixel);

	xcb_flush(c);

	setupEvents();

	// Event loop
	eventLoop();

	xcb_disconnect(c);

	return (0);
}
