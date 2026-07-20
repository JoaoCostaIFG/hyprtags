#include <string>

#include "../include/utils.hpp"

std::unordered_set<PHLWINDOW> getWindowsOnWorkspace(const uint32_t workspaceId) {
    std::unordered_set<PHLWINDOW> windows;

    for (auto& w : Desktop::windowState()->windows()) {
        if (w->workspaceID() == workspaceId && w->m_isMapped) {
            windows.insert(w);
        }
    }

    return windows;
}
