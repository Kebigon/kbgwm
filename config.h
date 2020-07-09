#define MODKEY XCB_MOD_MASK_4

#define FOCUS_COLOR 0xFF0000
#define UNFOCUS_COLOR 0x005577

#define BORDER_WIDTH 1

static const char* termcmd [] = { "alacritty",
	                              NULL };
static const char* menucmd [] = { "dmenu_run",
	                              NULL };

#define WORKSPACEKEYS(KEY,WORKSPACE) \
	{ MODKEY|XCB_MOD_MASK_SHIFT, KEY, sendToWorkspace, {.i = WORKSPACE} }, \
	{ MODKEY,                    KEY, changeWorkspace, {.i = WORKSPACE} },

static Key keys [] =
{
	{ MODKEY, XK_Return,    start,             { .cmd = termcmd }},
	{ MODKEY, XK_p,         start,             { .cmd = menucmd }},
	{ MODKEY, XK_Page_Up,   previousWorkspace, { 0 }},
	{ MODKEY, XK_Page_Down, nextWorkspace,     { 0 }},
	WORKSPACEKEYS(XK_Home, 0)
	WORKSPACEKEYS(XK_1,    0)
	WORKSPACEKEYS(XK_2,    1)
	WORKSPACEKEYS(XK_3,    2)
	WORKSPACEKEYS(XK_4,    3)
	WORKSPACEKEYS(XK_5,    4)
	WORKSPACEKEYS(XK_6,    5)
	WORKSPACEKEYS(XK_7,    6)
	WORKSPACEKEYS(XK_8,    7)
	WORKSPACEKEYS(XK_9,    8)
	WORKSPACEKEYS(XK_0,    9)
	WORKSPACEKEYS(XK_End,  9)
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
