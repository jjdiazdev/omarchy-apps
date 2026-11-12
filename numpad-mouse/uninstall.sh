#!/bin/bash
# Removes numouse: the daemon, the Hyprland config it added, and the package.
set -euo pipefail

HYPR_DIR="$HOME/.config/hypr"
CONFIG_FILE="$HYPR_DIR/numouse.lua"
BINDINGS_FILE="$HYPR_DIR/bindings.lua"

say() { printf '\033[1;34m==>\033[0m %s\n' "$*"; }

say "Stopping the daemon"
numouse disable >/dev/null 2>&1 || true
pkill -x numouse 2>/dev/null || true

if [[ -f "$BINDINGS_FILE" ]] && grep -qF 'require("hypr.numouse")' "$BINDINGS_FILE"; then
  say "Removing numouse from $BINDINGS_FILE"
  sed -i -e '/^-- Numpad mouse: see ~\/.config\/hypr\/numouse.lua$/d' \
         -e '/^require("hypr.numouse")$/d' "$BINDINGS_FILE"
fi

if [[ -f "$CONFIG_FILE" ]]; then
  say "Removing $CONFIG_FILE"
  rm "$CONFIG_FILE"
fi

if [[ -n "${HYPRLAND_INSTANCE_SIGNATURE:-}" ]]; then
  say "Reloading Hyprland"
  hyprctl reload >/dev/null
fi

if pacman -Q numouse >/dev/null 2>&1; then
  say "Removing the numouse package"
  sudo pacman -R --noconfirm numouse
fi

say "numouse is uninstalled."
