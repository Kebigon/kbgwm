#define MODKEY XCB_MOD_MASK_4

#define FOCUS_COLOR 0xFF0000
#define UNFOCUS_COLOR 0x005577

#define BORDER_WIDTH 1

static const char* termcmd [] = { "alacritty",
	                              NULL };
static const char* menucmd [] = { "dmenu_run",
	                              NULL };

static Key keys [] =
{
	{ MODKEY,
	  XK_Return,
	  start,
	  { .cmd = termcmd }},
	{ MODKEY,
	  XK_p,
	  start,
	  { .cmd = menucmd }}
};

static Button buttons [] =
{
	{ MODKEY,
	  XCB_BUTTON_INDEX_1,
	  mousemove,
	  { 0 }},
	{ MODKEY,
	  XCB_BUTTON_INDEX_3,
	  mouseresize,
	  { 0 }},
};