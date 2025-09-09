#include <string>

#include "../include/utils.hpp"

std::unordered_set<CWindow*> getWindowsOnWorkspace(const uint32_t workspaceId) {
    std::unordered_set<CWindow*> windows;

    for (auto& w : g_pCompositor->m_windows) {
        if (w->workspaceID() == workspaceId && w->m_isMapped) {
            windows.insert(w.get());
        }
    }

    return windows;
}
