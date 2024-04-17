#include <string>

#include "../include/utils.hpp"

static const char* SUPERSCRIPT_DIGITS[] = {"⁰", "¹", "²", "³", "⁴", "⁵", "⁶", "⁷", "⁸", "⁹"};

std::string        getWorkspaceName(CMonitor* monitor, const std::string& workspace) {
    if (monitor->ID > 0) {
        const std::string monitorIdStr = std::to_string(monitor->ID + 1);
        std::string       monitorExp   = "";
        for (int i = 0; i < workspace.length(); ++i) {
            monitorExp += SUPERSCRIPT_DIGITS[monitorIdStr[i] - '0'];
        }
        return workspace + monitorExp;
    }
    return workspace;
}

std::vector<CWindow*> getWindowsOnWorkspace(PHLWORKSPACE pWorkspace) {
    std::vector<CWindow*> windows;

    if (pWorkspace == nullptr) {
        return windows;
    }

    for (auto& w : g_pCompositor->m_vWindows) {
        if (w->workspaceID() == pWorkspace->m_iID && w->m_bIsMapped) {
            windows.push_back(w.get());
        }
    }

    return windows;
}
