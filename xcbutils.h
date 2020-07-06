#ifndef XCBUTILS_H_
#define XCBUTILS_H_

#define LENGTH( X ) (sizeof X / sizeof X [ 0 ])

#include <xcb/xcb_keysyms.h>
#include <xcb/xcb.h>

#include "types.h"

#define BUTTON_EVENT_MASK XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE

void*
emalloc
(
	size_t size
);

/*
 * registering events
 */
void
xcb_register_key_events
(
	Key key
);
void
xcb_register_button_events
(
	Button button
);

xcb_keycode_t*
xcb_get_keycodes
(
	xcb_keysym_t
);
xcb_keysym_t
xcb_get_keysym
(
	xcb_keycode_t
);

#endif /* XCBUTILS_H_ */