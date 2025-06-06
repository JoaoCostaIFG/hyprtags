#ifndef HYPRTAGS_UTILS_H
#define HYPRTAGS_UTILS_H

#include <unordered_set>

#define WLR_USE_UNSTABLE

#include <hyprland/src/includes.hpp>

#include <hyprland/src/Compositor.hpp>

#define private public
#include <hyprland/src/helpers/Monitor.hpp>
#include <hyprland/src/desktop/Window.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/desktop/DesktopTypes.hpp>
#undef private

#define HYPRTAGS "[hyprtags]"

#define GET_CURRENT_MONITOR()         g_pCompositor->getMonitorFromCursor()
#define GET_ACTIVE_WORKSPACE()        GET_CURRENT_MONITOR()->m_activeWorkspace
#define GET_ACTIVE_SPECIALWORKSPACE() GET_CURRENT_MONITOR()->activeSpecialWorkspace
#define GET_LAST_WINDOW()             g_pCompositor->m_pLastWindow

// Returns the current active special workspace (if any) or the active 'normal' workspace
static inline PHLWORKSPACE getActiveWorkspace() {
    auto monitor = GET_CURRENT_MONITOR();
    if (monitor->m_activeSpecialWorkspace != nullptr) {
        return monitor->m_activeSpecialWorkspace;
    }
    return monitor->m_activeWorkspace;
}

std::unordered_set<CWindow*> getWindowsOnWorkspace(const std::string& workspaceName);

#endif //HYPRTAGS_UTILS_H
