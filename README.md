# kbgwm, sucklessy floating window manager

kbgwm is:
- floating / stacking, if you want to try a tilling WM, you can try [dwm](https://dwm.suckless.org/)
- click-to-focus, if you want to try a sloppy-focus WM, you can try [mcwm](https://hack.org/mc/projects/mcwm)
- non-reparenting
- sucklessy, to change the configuration, just edit config.h to your liking and recompile

## Current state

kbgwm is still under active development, although perfectly fonctional, it still lacks some features you would expect from a window manager like toolbar handling and multi-monitors. It also still has debug logs.

## Default shortcuts

By default the is the ALT key (MOD1), you can also set it to the super key (MOD4)

| Shortcut                | Action                                |
| ----------------------- | ------------------------------------- |
| MOD + Return            | Start xterm                           |
| MOD + p                 | Start dmenu                           |
| MOD + Tab               | Focus the next window                 |
| MOD + SHIFT + Tab       | Focus the previous window             |
| MOD + x                 | Maximize/unmaximize window            |
| MOD + q                 | Close window                          |
| MOD + SHIFT + q         | Close kbgwm                           |
| MOD + [0-9]             | Go to workspace #                     |
| MOD + Home              | Go to workspace 1                     |
| MOD + Page Up           | Go to the previous workspace          |
| MOD + Page Down         | Go to the next workspace              |
| MOD + End               | Go to workspace 10                    |
| MOD + SHIFT + [0-9]     | Move window to workspace #            |
| MOD + SHIFT + Home      | Move window to workspace 1            |
| MOD + SHIFT + Page Up   | Move window to the previous workspace |
| MOD + SHIFT + Page Down | Move window to the next workspace     |
| MOD + SHIFT + End       | Move window to workspace 10           |
| MOD + Left Click        | Move window                           |
| MOD + Right Click       | Resize window                         |

You can edit all those settings via the config.h file.

## Thanks

- Thanks to the [suckless](https://suckless.org) project
- Thanks to venam for creating [2bwm](https://github.com/venam/2bwm)
- Thanks to Michael Cardell for creating [mcwm](https://hack.org/mc/projects/mcwm)
