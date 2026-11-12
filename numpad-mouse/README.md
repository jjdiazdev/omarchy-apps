# Numpad Mouse (`numouse`)

Drive the mouse pointer from the numeric keypad on Omarchy, so you don't have
to reach for the mouse.

numouse has no window. Toggling it shows one desktop notification:
**Numpad Mouse Actived** or **Numpad Mouse Deactivated**.

## Keys

| Key | Action |
| --- | --- |
| Super + KP_Divide (`/` on the keypad) | Toggle numpad mouse |
| Super + Alt + T | Toggle numpad mouse |
| 8 / 2 / 4 / 6 | Move up / down / left / right |
| 7 / 9 / 1 / 3 | Move diagonally |
| 5 | Left click |
| 5 twice quickly | Right click |
| + | Scroll up (hold to repeat) |
| Enter | Scroll down (hold to repeat) |

- Holding a direction accelerates. Holding two keys (say 8 and 6) combines them.
- A single 5 clicks 250 ms after the press, because it has to wait to rule out
  a double press.
- The keys work with NumLock on or off.
- While the mode is off, the keypad types digits as usual.

## Requirements

- Omarchy with the Lua Hyprland config (Hyprland 0.56 or newer). Check that
  `~/.config/hypr/hyprland.lua` exists; if not, run `omarchy update` first.
- An x86_64 or aarch64 machine.
- Internet access for `pacman` to fetch build dependencies (`base-devel`,
  `qt6-base`, `wayland`, `libnotify`); Omarchy already ships most of them.

## Install

Run these from a terminal inside your Hyprland session (Super + Enter), not
from a tool without a terminal, because `sudo` asks for your password:

```bash
git clone <URL-of-this-repo> ~/Work/omarchy-apps
cd ~/Work/omarchy-apps/numpad-mouse
./install.sh
```

Without git, copy the `numpad-mouse` folder to the other machine (USB,
LocalSend, `scp`) and run `./install.sh` from inside it.

`install.sh` does the following:

1. Checks that this is an Omarchy system with the Lua Hyprland config, and
   warns if Super + KP_Divide or Super + Alt + T are already in use.
2. Installs the build dependencies with `pacman --needed`.
3. Builds the app from source and installs it as the `numouse` package with
   `makepkg -fsi`.
4. Copies `hypr/numouse.lua` to `~/.config/hypr/numouse.lua` (backing up a
   different existing copy) and adds `require("hypr.numouse")` to
   `~/.config/hypr/bindings.lua` once.
5. Reloads Hyprland and starts the daemon. From then on the daemon starts by
   itself at login.

Then press **Super + KP_Divide** to try it.

### Manual install

If you prefer to run each step yourself:

```bash
sudo pacman -S --needed base-devel qt6-base wayland libnotify
cd numpad-mouse/pkgbuild && makepkg -fsi && cd ..
cp hypr/numouse.lua ~/.config/hypr/numouse.lua
printf '\nrequire("hypr.numouse")\n' >> ~/.config/hypr/bindings.lua
hyprctl reload && setsid -f numouse daemon
```

## Update

Pull the new version and run the installer again. It rebuilds the package,
refreshes `~/.config/hypr/numouse.lua`, and restarts the daemon:

```bash
cd ~/Work/omarchy-apps && git pull
cd numpad-mouse && ./install.sh
```

## Uninstall

```bash
cd ~/Work/omarchy-apps/numpad-mouse
./uninstall.sh
```

It stops the daemon, removes `~/.config/hypr/numouse.lua` and the `require`
line, reloads Hyprland, and removes the package.

## Customize

- **Shortcuts:** edit the two `o.bind(...)` lines at the end of
  `~/.config/hypr/numouse.lua`, then run `hyprctl reload`. Note that
  `./install.sh` overwrites this file (keeping a `.bak` copy).
- **Speed, scroll rate, double-press time:** change `MinSpeed`, `MaxSpeed`,
  `RampMs`, `ScrollDelayMs`, `ScrollRepeatMs`, or `DoubleClickMs` in
  `src/controller.h`, then run `./install.sh` again.

## Troubleshooting

| Problem | Fix |
| --- | --- |
| Notification "the daemon is not running" | Run `setsid -f numouse daemon`. To see errors, run `numouse daemon` in a terminal. |
| The shortcut does nothing | Run `hyprctl configerrors`, and check `grep numouse ~/.config/hypr/bindings.lua`. |
| Mode seems inverted after `hyprctl reload` | Reloading resets the shortcut's state while the daemon keeps its own. Press the shortcut twice. |
| `numouse status` | Prints `active` or `inactive`. |

## How it works

- `numouse daemon` runs in the background and creates a pointer device through
  the `zwlr_virtual_pointer_v1` Wayland protocol. That needs no root, uinput,
  or `input` group.
- `~/.config/hypr/numouse.lua` binds the keypad only while the mode is on.
  Each key down/up runs `numouse press <key>` / `numouse release <key>`, which
  talks to the daemon over `$XDG_RUNTIME_DIR/numouse.sock`.

### Commands

```
numouse daemon            run the background service
numouse toggle            turn the mode on or off
numouse enable | disable
numouse press <key>       a keypad key went down (1-9, add, enter)
numouse release <key>     a keypad key went up
numouse status            print active or inactive
```

## Development

```
bin/build     # compile into build/numouse
bin/test      # run the Qt Test suite
bin/install   # build and install the package (makepkg -fsi)
```

## Files

```
install.sh / uninstall.sh   one-step install and removal
hypr/numouse.lua            Hyprland shortcuts and daemon autostart
numouse.pro                 qmake project
src/                        daemon, client, virtual pointer, controller
protocol/                   wlr virtual pointer protocol XML
tests/                      Qt Test suite
pkgbuild/                   Arch package (PKGBUILD, icon)
bin/                        build, test, and install scripts
```

## License

MIT. See [LICENSE](LICENSE).
