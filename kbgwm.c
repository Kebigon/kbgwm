#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <xcb/xcb.h>
#include <X11/keysym.h>

#include "xcbutils.h"

#define IGNORE_LOCK(modifier) (modifier & ~(XCB_MOD_MASK_LOCK))

static void start(const Arg* arg);

#include "config.h"

bool quit = false;
xcb_connection_t* c;
xcb_window_t root;

void setupEvents()
{
	/*
	 * Get all the key symbols
	 */

//	xcb_grab_key(c, 1, root, XCB_MOD_MASK_ANY, XCB_GRAB_ANY,
//			XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

	xcb_grab_button(c, 0, root, XCB_EVENT_MASK_BUTTON_PRESS |
	                XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
	                XCB_GRAB_MODE_ASYNC, root, XCB_NONE, 1, XCB_MOD_MASK_ANY);

	xcb_grab_button(c, 0, root, XCB_EVENT_MASK_BUTTON_PRESS |
	                XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
	                XCB_GRAB_MODE_ASYNC, root, XCB_NONE, 3, XCB_MOD_MASK_ANY);

	for (uint_fast8_t i = 0; i != LENGTH(keys); i++)
	{
		xcb_register_key_press_events(keys[i]);
	}

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
		printf("testing key: mod %d key %d\n", keys[i].modifiers, keys[i].keysym);

		if (keysym == keys[i].keysym && (event->state & keys[i].modifiers) == keys[i].modifiers)
		{
			printf("Key found !\n");
			keys[i].func(&keys[i].arg);
			break;
		}
	}
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

			case XCB_MAPPING_NOTIFY:
				printf("\nXCB_MAPPING_NOTIFY\n");
				onMappingNotify((xcb_mapping_notify_event_t*) event);
				break;

			default:
				printf("Received event, response type %d\n", event->response_type & ~0x80);
				break;
		}

		free(event);
	}
}

void start(const Arg* arg)
{
	if (fork() == 0)
	{
		// Child process
		setsid();
		execvp((char*)arg->cmd[0], (char**)arg->cmd);
	}
}

int main(void)
{
	/*
	 * displayname = NULL -> use DISPLAY environment variable
	 */
	int screenp;
	c = xcb_connect(NULL, &screenp);
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

	xcb_screen_t* screen = iter.data;
	if (!screen)
	{
		xcb_disconnect(c);
		perror("screen not found");
		exit(1);
	}

	root = screen->root;

	xcb_flush(c);

	setupEvents();

	// Event loop
	eventLoop();

	xcb_disconnect(c);

	return 0;
}
