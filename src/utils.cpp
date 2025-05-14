#include <string>

#include "../include/utils.hpp"

static const char*           SUPERSCRIPT_DIGITS[] = {"⁰", "¹", "²", "³", "⁴", "⁵", "⁶", "⁷", "⁸", "⁹"};

std::unordered_set<CWindow*> getWindowsOnWorkspace(const std::string& workspaceName) {
    std::unordered_set<CWindow*> windows;

    PHLWORKSPACE                 pWorkspace = g_pCompositor->getWorkspaceByName(workspaceName);
    if (pWorkspace == nullptr) {
        return windows;
    }

    for (auto& w : g_pCompositor->m_windows) {
        if (w->workspaceID() == pWorkspace->m_id && w->m_isMapped) {
            windows.insert(w.get());
        }
    }

    return windows;
}
