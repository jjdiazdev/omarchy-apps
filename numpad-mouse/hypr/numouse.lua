-- Numpad mouse (numouse) for Omarchy / Hyprland.
--
-- Installed by numpad-mouse/install.sh as ~/.config/hypr/numouse.lua and loaded
-- from ~/.config/hypr/bindings.lua with require("hypr.numouse").
--
-- Super + KP_Divide or Super + Alt + T toggles the mode. While it is on, the
-- keypad digits drive the pointer, 5 clicks, + scrolls up and Enter scrolls
-- down. While it is off, the keypad types as usual.

-- Start the background daemon with the session. A reload does not fire this
-- again, so the daemon is never started twice.
o.exec_on_start("numouse daemon")

do
  local numouse_active = false
  local numouse_binds = {}

  -- Each entry lists the key names that should act alike, then the key the
  -- daemon is told about. Digits have a second name when NumLock is off.
  local numouse_keys = {
    { "KP_1", "KP_End", 1 }, { "KP_2", "KP_Down", 2 }, { "KP_3", "KP_Next", 3 },
    { "KP_4", "KP_Left", 4 }, { "KP_5", "KP_Begin", 5 }, { "KP_6", "KP_Right", 6 },
    { "KP_7", "KP_Home", 7 }, { "KP_8", "KP_Up", 8 }, { "KP_9", "KP_Prior", 9 },
    -- + scrolls up and Enter scrolls down; NumLock does not rename these two.
    { "KP_Add", "add" }, { "KP_Enter", "enter" },
  }

  -- Any unbind of the chord expires the handle; Hyprland 0.56 only tolerates
  -- tostring() on an expired one.
  local function alive(keybind)
    return keybind ~= nil and tostring(keybind) ~= "HL.Keybind(expired)"
  end

  local function set_keypad(enabled)
    if enabled and #numouse_binds == 0 then
      for _, entry in ipairs(numouse_keys) do
        local key_id = entry[#entry]
        for i = 1, #entry - 1 do
          local key = entry[i]
          table.insert(numouse_binds, hl.bind(key, hl.dsp.exec_cmd("numouse press " .. key_id)))
          -- Like o.bind_hold: Hyprland drops a plain release bind when another
          -- key is released during the hold, which would leave the pointer
          -- drifting, so the release half is transparent and ignores modifiers.
          table.insert(numouse_binds, hl.bind(key, hl.dsp.exec_cmd("numouse release " .. key_id),
            { release = true, transparent = true, ignore_mods = true, non_consuming = true }))
        end
      end
    end
    for _, keybind in ipairs(numouse_binds) do
      if alive(keybind) then
        keybind:set_enabled(enabled)
      end
    end
  end

  local function toggle()
    numouse_active = not numouse_active
    set_keypad(numouse_active)
    hl.exec_cmd(numouse_active and "numouse enable" or "numouse disable")
  end

  o.bind("SUPER + KP_Divide", "Toggle numpad mouse", toggle)
  o.bind("SUPER + ALT + T", "Toggle numpad mouse", toggle)
end
