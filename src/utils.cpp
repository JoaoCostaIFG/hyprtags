#include <format>
#include <string>

#include "../include/utils.hpp"

std::unordered_set<PHLWINDOW> getWindowsOnWorkspace(const uint32_t workspaceId) {
    std::unordered_set<PHLWINDOW> windows;
    const std::string             wantedAddress = std::to_string(workspaceId);

    for (auto& w : Desktop::windowState()->windows()) {
        if (!w->mapped()) {
            continue;
        }

        // the addressable name of a numbered workspace is its decimal ID
        if (w->workspaceAddress() == wantedAddress) {
            windows.insert(w);
        }
    }

    return windows;
}

// Recent Hyprland versions replaced the string-based keybind dispatchers with typed
// Config::Actions functions returning std::expected. Convert back to SDispatchResult so
// the rest of the plugin keeps working with a single result type.
static SDispatchResult toDispatchResult(const Config::Actions::ActionResult& result) {
    if (result.has_value()) {
        return SDispatchResult{.passEvent = result->passEvent};
    }
    return SDispatchResult{.success = false, .error = std::format(HYPRTAGS " {}", result.error().message)};
}

SDispatchResult changeWorkspace(const std::string& ws) {
    return toDispatchResult(Config::Actions::changeWorkspace(ws));
}

SDispatchResult moveWindowToWorkspace(const std::string& ws, bool silent, std::optional<PHLWINDOW> window) {
    return toDispatchResult(Config::Actions::moveToWorkspace(ws, silent, window));
}

SDispatchResult moveWindowToWorkspace(PHLWORKSPACE ws, bool silent, std::optional<PHLWINDOW> window) {
    return toDispatchResult(Config::Actions::moveToWorkspace(ws, silent, window));
}
