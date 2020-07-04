
#define MODKEY XCB_MOD_MASK_4

static const char* termcmd[] = {"alacritty", NULL};

Key keys[] =
{
	{ MODKEY, XK_Return, start, { .cmd = termcmd } }
};
