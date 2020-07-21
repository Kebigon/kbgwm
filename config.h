#define MODKEY XCB_MOD_MASK_4
#define SHIFT  XCB_MOD_MASK_SHIFT

#define FOCUS_COLOR 0xFF0000
#define UNFOCUS_COLOR 0x005577

#define BORDER_WIDTH 1

/*
 * Number of workspaces
 * They will be numbered from 0 to NB_WORKSPACES-1
 */
#define NB_WORKSPACES 10

static const char* termcmd[] = { "xterm", NULL };
static const char* menucmd[] = { "dmenu_run", NULL };

#define WORKSPACEKEYS(KEY,WORKSPACE) \
	{ MODKEY|XCB_MOD_MASK_SHIFT, KEY, workspace_send,   {.i = WORKSPACE} }, \
	{ MODKEY,                    KEY, workspace_change, {.i = WORKSPACE} },

static Key keys[] =
{
	{ MODKEY,         XK_Return,    start,                  { .cmd = termcmd } },
	{ MODKEY,         XK_p,         start,                  { .cmd = menucmd } },
	{ MODKEY,         XK_Page_Up,   workspace_previous,     { 0 } },
	{ MODKEY,         XK_Page_Down, workspace_next,         { 0 } },
	{ MODKEY | SHIFT, XK_Tab,       focus_next,             { .b = true } },
	{ MODKEY,         XK_Tab,       focus_next,             { .b = false } },
	{ MODKEY,         XK_q,         client_kill,            { 0 } },
	{ MODKEY | SHIFT, XK_q,         quit,                   { 0 } },
	{ MODKEY,         XK_x,         client_toggle_maximize, { 0 } },
	WORKSPACEKEYS(XK_Home, 0)
	WORKSPACEKEYS(XK_1, 0)
	WORKSPACEKEYS(XK_2, 1)
	WORKSPACEKEYS(XK_3, 2)
	WORKSPACEKEYS(XK_4, 3)
	WORKSPACEKEYS(XK_5, 4)
	WORKSPACEKEYS(XK_6, 5)
	WORKSPACEKEYS(XK_7, 6)
	WORKSPACEKEYS(XK_8, 7)
	WORKSPACEKEYS(XK_9, 8)
	WORKSPACEKEYS(XK_0, 9)
	WORKSPACEKEYS(XK_End, 9)
};

static Button buttons[] =
{
	{ MODKEY, XCB_BUTTON_INDEX_1, mousemove,   { 0 } },
	{ MODKEY, XCB_BUTTON_INDEX_3, mouseresize, { 0 } },
};
