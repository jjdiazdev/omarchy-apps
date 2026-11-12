#!/bin/bash
# Installs numouse (numpad mouse) on an Omarchy system from source:
# builds and installs the Arch package, installs the Hyprland config, and
# starts the daemon. Safe to run again to update.
set -euo pipefail

APP_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
HYPR_DIR="$HOME/.config/hypr"
CONFIG_FILE="$HYPR_DIR/numouse.lua"
BINDINGS_FILE="$HYPR_DIR/bindings.lua"
REQUIRE_LINE='require("hypr.numouse")'

say() { printf '\033[1;34m==>\033[0m %s\n' "$*"; }
fail() { printf '\033[1;31mError:\033[0m %s\n' "$*" >&2; exit 1; }

# --- Checks -----------------------------------------------------------------

[[ $EUID -ne 0 ]] || fail "Run this as your normal user, not root. It asks for sudo when needed."
command -v pacman >/dev/null || fail "pacman not found. This installer is for Omarchy (Arch Linux)."
command -v hyprctl >/dev/null || fail "hyprctl not found. Hyprland is required."
[[ -f "$HYPR_DIR/hyprland.lua" ]] || fail "$HYPR_DIR/hyprland.lua not found. numouse needs Omarchy with the Lua Hyprland config (Hyprland 0.56 or newer). Update with: omarchy update"
[[ -f "$BINDINGS_FILE" ]] || fail "$BINDINGS_FILE not found."

# Warn when another binding already uses one of the toggle chords.
if command -v omarchy >/dev/null; then
  taken=$(omarchy menu keybindings --print 2>/dev/null \
    | grep -E '^(SUPER \+ KP_DIVIDE|SUPER \+ ALT \+ T) ' | grep -vi 'numpad mouse' || true)
  if [[ -n "$taken" ]]; then
    printf '\033[1;33mWarning:\033[0m these shortcuts are already in use and will be shared:\n%s\n' "$taken"
    printf 'Edit %s after installing to pick other keys.\n' "$CONFIG_FILE"
  fi
fi

# --- Build and install the package -----------------------------------------

say "Installing build dependencies"
sudo pacman -S --needed --noconfirm base-devel qt6-base wayland libnotify

say "Building and installing the numouse package"
rm -rf "$APP_DIR/build"
(cd "$APP_DIR/pkgbuild" && makepkg -fsi --noconfirm)

# --- Hyprland config --------------------------------------------------------

if [[ -f "$CONFIG_FILE" ]] && ! cmp -s "$APP_DIR/hypr/numouse.lua" "$CONFIG_FILE"; then
  backup="$CONFIG_FILE.bak.$(date +%Y%m%d%H%M%S)"
  say "Backing up your existing $CONFIG_FILE to $backup"
  cp "$CONFIG_FILE" "$backup"
fi
say "Installing $CONFIG_FILE"
install -Dm644 "$APP_DIR/hypr/numouse.lua" "$CONFIG_FILE"

if grep -qF "$REQUIRE_LINE" "$BINDINGS_FILE"; then
  say "$BINDINGS_FILE already loads numouse"
else
  say "Adding $REQUIRE_LINE to $BINDINGS_FILE"
  printf '\n-- Numpad mouse: see ~/.config/hypr/numouse.lua\n%s\n' "$REQUIRE_LINE" >> "$BINDINGS_FILE"
fi

# --- Start ------------------------------------------------------------------

if [[ -n "${HYPRLAND_INSTANCE_SIGNATURE:-}" ]]; then
  say "Reloading Hyprland"
  hyprctl reload >/dev/null
  errors=$(hyprctl configerrors 2>/dev/null || true)
  [[ -z "${errors//[[:space:]]/}" ]] || printf '\033[1;33mWarning:\033[0m Hyprland reports config errors:\n%s\n' "$errors"

  # Restart the daemon so an update runs the new binary.
  pkill -x numouse 2>/dev/null || true
  sleep 0.3
  setsid -f numouse daemon >/dev/null 2>&1
  sleep 0.3
  if numouse status >/dev/null 2>&1; then
    say "numouse daemon is running"
  else
    fail "The daemon did not start. Run 'numouse daemon' in a terminal to see why."
  fi
else
  say "Not inside a Hyprland session. Log in to Hyprland; the daemon starts on its own."
fi

say "Done. Press Super + KP_Divide (or Super + Alt + T) to toggle the numpad mouse."
