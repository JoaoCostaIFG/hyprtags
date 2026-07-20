#include <format>
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

SDispatchResult runDispatcher(const std::string& name, const std::string& args) {
    if (!g_pKeybindManager) {
        return SDispatchResult{.success = false, .error = std::format("[hyprtags] keybind manager unavailable for dispatcher '{}'", name)};
    }

    const auto it = g_pKeybindManager->m_dispatchers.find(name);
    if (it == g_pKeybindManager->m_dispatchers.end()) {
        return SDispatchResult{.success = false, .error = std::format("[hyprtags] dispatcher '{}' not found", name)};
    }

    return it->second(args);
}
