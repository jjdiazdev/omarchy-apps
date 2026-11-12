# omarchy-apps

Small apps and utilities for [Omarchy](https://omarchy.org), each ready to
install cleanly on any Omarchy machine.

Every app lives in its own folder with its full source, its Hyprland config,
and an `install.sh` that builds and installs it from scratch. Nothing is
prebuilt, so each machine compiles against its own libraries.

## Apps

| App | Folder | What it does |
| --- | --- | --- |
| Numpad Mouse (`numouse`) | [`numpad-mouse/`](numpad-mouse/) | Move the pointer, click, and scroll with the numeric keypad. Toggle with Super + KP_Divide. |

## Quick start

From a terminal inside your Hyprland session (Super + Enter):

```bash
git clone <URL-of-this-repo> ~/Work/omarchy-apps
cd ~/Work/omarchy-apps/<app-folder>
./install.sh
```

Each app's README lists its keys, requirements, and manual steps.

### Update

```bash
cd ~/Work/omarchy-apps && git pull
cd <app-folder> && ./install.sh
```

### Uninstall

```bash
cd ~/Work/omarchy-apps/<app-folder>
./uninstall.sh
```

## Requirements

- Omarchy with the Lua Hyprland config (Hyprland 0.56 or newer). If
  `~/.config/hypr/hyprland.lua` is missing, run `omarchy update` first.
- Internet access, so `pacman` can fetch build dependencies.
- Run the installers as your normal user. They ask for `sudo` only to install
  packages.

## How the apps are installed

- Each app installs as a regular Arch package built with `makepkg`, so
  `pacman -Q <name>` shows it and `pacman -R <name>` removes it.
- Hyprland config goes in its own file, `~/.config/hypr/<name>.lua`, loaded
  by one `require("hypr.<name>")` line in `~/.config/hypr/bindings.lua`. The
  installers never edit Omarchy's own defaults in `/usr/share/omarchy`.
- Running `install.sh` again updates the app. It backs up a modified config
  file before replacing it.

## Adding a new app

1. Create a folder named after the app.
2. Put in it the full source, `pkgbuild/PKGBUILD`, any Hyprland config under
   `hypr/`, a `README.md`, an `install.sh`, and an `uninstall.sh`.
   `numpad-mouse/` is the template to copy.
3. Test `./install.sh` and `./uninstall.sh` on a machine without the app.
4. Add a row to the table above.

## License

MIT. See [LICENSE](LICENSE). Each app folder carries its own LICENSE as well.
