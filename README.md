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

Afterwards, I recommend adding `exec-once = hyprpm reload -n` to your `hyprland.conf`.

### Manual

1. Clone this repository
2. Use `make` to build the plugin (inside the repo directory)
3. Call `hyprctl plugin load $(pwd)/hyprtags.so`

You can also use `hyprpm` as shown above to download and build the plugin and then add `plugin=~/.local/share/hyprpm/hyprtags/hyprtags.so` to `hyprland.conf` (I think, haven't tested it).

## Usage

I just changed the default dispatchers for dealing with windows and workspaces to these new replacements:

```sh
#
# Workspaces:
#
# Switch workspaces with mod + [0-9]
bind = $mod, 1, tags-workspace, 1
bind = $mod, 2, tags-workspace, 2
bind = $mod, 3, tags-workspace, 3
bind = $mod, 4, tags-workspace, 4
bind = $mod, 5, tags-workspace, 5
bind = $mod, 6, tags-workspace, 6
bind = $mod, 7, tags-workspace, 7
bind = $mod, 8, tags-workspace, 8
bind = $mod, 9, tags-workspace, 9
# Move active window to a workspace with mod + SHIFT + [0-9]
bind = $mod+SHIFT, 1, tags-movetoworkspacesilent, 1
bind = $mod+SHIFT, 2, tags-movetoworkspacesilent, 2
bind = $mod+SHIFT, 3, tags-movetoworkspacesilent, 3
bind = $mod+SHIFT, 4, tags-movetoworkspacesilent, 4
bind = $mod+SHIFT, 5, tags-movetoworkspacesilent, 5
bind = $mod+SHIFT, 6, tags-movetoworkspacesilent, 6
bind = $mod+SHIFT, 7, tags-movetoworkspacesilent, 7
bind = $mod+SHIFT, 8, tags-movetoworkspacesilent, 8
bind = $mod+SHIFT, 9, tags-movetoworkspacesilent, 9
# Borrow workspaces
bind = $mod+CONTROL, 1, tags-toggleworkspace, 1
bind = $mod+CONTROL, 2, tags-toggleworkspace, 2
bind = $mod+CONTROL, 3, tags-toggleworkspace, 3
bind = $mod+CONTROL, 4, tags-toggleworkspace, 4
bind = $mod+CONTROL, 5, tags-toggleworkspace, 5
bind = $mod+CONTROL, 6, tags-toggleworkspace, 6
bind = $mod+CONTROL, 7, tags-toggleworkspace, 7
bind = $mod+CONTROL, 8, tags-toggleworkspace, 8
bind = $mod+CONTROL, 9, tags-toggleworkspace, 9
# workspace alt-tab
bind = $mod, TAB, tags-workspacealttab,

# Example special workspace (scratchpad)
bind = $mod, Q, togglespecialworkspace, magic
bind = $mod+SHIFT, Q, tags-movetoworkspacesilent, special:magic
```

The plugin reloads your configuration after loading, so you can see a message flash telling you that the dispatchers above haven't been found.

## Limitations/TODO

I'm happy with the plugin for now, but there are some limitations/issues:

- Haven't tested disconnecting/connecting monitors. Probably will need to work on that
- Moving windows between monitors might cause some issues if the window is borrowed.
- Manually dragging floating windows to other monitors/workspaces can cause issues.
- The order of the windows can change when they go back to their original workspace.
- It would be nice to have an indication of which tags are currently active.

## License

See the [LICENSE](./LICENSE) file for details.
