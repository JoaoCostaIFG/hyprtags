#ifndef HYPRTAGS_UTILS_H
#define HYPRTAGS_UTILS_H

#include <vector>

#define WLR_USE_UNSTABLE

#include <hyprland/src/includes.hpp>

#define private public
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/helpers/Monitor.hpp>
#include <hyprland/src/desktop/Window.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#undef private

#define HYPRTAGS "[hyprtags]"

#define GET_CURRENT_MONITOR() g_pCompositor->getMonitorFromCursor()
#define GET_ACTIVE_WORKSPACE() GET_CURRENT_MONITOR()->activeWorkspace

std::string getWorkspaceName(CMonitor* monitor, const std::string& workspace);

std::vector<CWindow*> getWindowsOnWorkspace(PHLWORKSPACE pWorkspace);

#endif //HYPRTAGS_UTILS_H
