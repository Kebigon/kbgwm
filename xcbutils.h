#ifndef XCBUTILS_H_
#define XCBUTILS_H_

#define LENGTH(X) (sizeof X / sizeof X[0])

#include <xcb/xcb_keysyms.h>
#include <xcb/xcb.h>

#include "types.h"

/*
 * registering events
 */
void xcb_register_key_press_events(Key key);

xcb_keycode_t* xcb_get_keycodes(xcb_keysym_t);
xcb_keysym_t xcb_get_keysym(xcb_keycode_t);

#endif /* XCBUTILS_H_ */
