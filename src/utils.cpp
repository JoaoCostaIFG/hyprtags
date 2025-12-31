#include <string>

#include "../include/utils.hpp"

std::unordered_set<Desktop::View::CWindow*> getWindowsOnWorkspace(const uint32_t workspaceId) {
    std::unordered_set<Desktop::View::CWindow*> windows;

    for (auto& w : g_pCompositor->m_windows) {
        if (w->workspaceID() == workspaceId && w->m_isMapped) {
            windows.insert(w.get());
        }
    }

    return windows;
}
