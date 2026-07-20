# Hyprtags

Hyprtags is a [Hyprland plugin](https://wiki.hyprland.org/Plugins/Using-Plugins/) that tries to emulate the workspace tag system from [DWM](https://dwm.suckless.org/). In short, in DWM you have 9 workspaces called tags. Windows can belong to one or more tags. You activate the tags you want to see right now and all windows belonging to those are shown.

This implementation is a bit of an hybrid:

- You still get access to special and named workspaces as usual.
- Windows can only belong to one tag at a time.
- You can see multiple tags at once

## Installation

### Hyprpm

With hyprpm you just need to do the following:

```sh
hyprpm update
hyprpm add "https://github.com/JoaoCostaIFG/hyprtags"
hyprpm enable hyprtags
```

Afterwards, I recommend adding `exec-once = hyprpm reload -n` to your `hyprland.lua`.

> **Note:** Hyprtags now requires a Lua config (`hyprland.lua`). The legacy `hyprland.conf`/hyprlang format is no longer supported because Hyprland's `hyprctl dispatch` does not expose plugin dispatchers (registered via `addDispatcherV2`) under the Lua `hl.dsp` namespace. The plugin therefore exposes its dispatchers as Lua functions under `hl.plugin.hyprtags.*`.

### Manual

1. Clone this repository
2. Use `make` to build the plugin (inside the repo directory)
3. Call `hyprctl plugin load $(pwd)/hyprtags.so`

You can also use `hyprpm` as shown above to download and build the plugin and then add `plugin=~/.local/share/hyprpm/hyprtags/hyprtags.so` to `hyprland.conf` (I think, haven't tested it).

## Usage

Load the plugin generated config by adding the following line to your `hyprland.lua`:

```lua
dofile(os.getenv("XDG_RUNTIME_DIR") ..
       "/hypr/" ..
       os.getenv("HYPRLAND_INSTANCE_SIGNATURE") ..
       "/hyprtags.lua")
```

This config file is automatically created when the plugin loads and cleaned up when unloaded.

The plugin exposes its dispatchers as Lua functions under `hl.plugin.hyprtags.*`:

| Function                          | Replaces (legacy)               | Description                                       |
| --------------------------------- | ------------------------------- | ------------------------------------------------- |
| `tags_workspace(arg)`             | `tags-workspace`                | Switch to a workspace tag (1-9)                   |
| `tags_workspace_alt_tab()`         | `tags-workspacealttab`          | Alternate-tag (recall last tag combination)       |
| `tags_move_to_workspace(arg)`      | `tags-movetoworkspace`          | Move the focused window to a tag, then switch     |
| `tags_move_to_workspace_silent(arg)` | `tags-movetoworkspacesilent`  | Move the focused window to a tag silently         |
| `tags_toggle_workspace(arg)`       | `tags-toggleworkspace`          | Toggle (borrow/unborrow) a tag                    |

Each returns a table `{ ok: boolean, pass_event: boolean, error?: string }`.

### Keybinds

```lua
local mod = "SUPER"

-- Switch workspaces with mod + [1-9]
for i = 1, 9 do
    hl.bind(mod .. "+" .. tostring(i), function() hl.plugin.hyprtags.tags_workspace(tostring(i)) end)
end

-- Move active window to a workspace with mod + SHIFT + [1-9]
for i = 1, 9 do
    hl.bind(mod .. "+SHIFT+" .. tostring(i), function() hl.plugin.hyprtags.tags_move_to_workspace_silent(tostring(i)) end)
end

-- Borrow workspaces with mod + CTRL + [1-9]
for i = 1, 9 do
    hl.bind(mod .. "+CTRL+" .. tostring(i), function() hl.plugin.hyprtags.tags_toggle_workspace(tostring(i)) end)
end

-- workspace alt-tab
hl.bind(mod .. "+TAB", function() hl.plugin.hyprtags.tags_workspace_alt_tab() end)

-- Example special workspace (scratchpad)
hl.bind(mod .. "+Q", function() hl.dispatch(hl.dsp.workspace.toggle_special("magic")) end)
hl.bind(mod .. "+SHIFT+Q", function() hl.plugin.hyprtags.tags_move_to_workspace_silent("special:magic") end)
```

### IPC

To trigger a dispatcher from outside Hyprland (e.g. a script or status bar), use `hyprctl eval`:

```sh
hyprctl eval 'hl.plugin.hyprtags.tags_workspace("2")'
```

The legacy `hyprctl dispatch tags-workspace 2` form is no longer supported — `hyprctl dispatch` in Lua configs only resolves built-in dispatchers from `hl.dsp`, not plugin-registered ones.

## Limitations/TODO

I'm happy with the plugin for now, but there are some limitations/issues:

- ~Haven't tested disconnecting/connecting monitors. Probably will need to work on that~
- Moving windows between monitors might cause some issues if the window is borrowed.
- Manually dragging floating windows to other monitors/workspaces can cause issues.
- The order of the windows can change when they go back to their original workspace.
- It would be nice to have an indication of which tags are currently active.

## License

See the [LICENSE](./LICENSE) file for details.
