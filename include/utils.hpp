#ifndef HYPRTAGS_UTILS_H
#define HYPRTAGS_UTILS_H

#include <charconv>
#include <cstdint>
#include <optional>
#include <string_view>
#include <unordered_set>

#define WLR_USE_UNSTABLE

#include <hyprland/src/includes.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/managers/input/InputManager.hpp>
#include <hyprland/src/state/MonitorState.hpp>
#include <hyprland/src/desktop/state/WindowState.hpp>

#define private public
#include <hyprland/src/output/Monitor.hpp>
#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/desktop/DesktopTypes.hpp>
#undef private

#define HYPRTAGS "[hyprtags]"

// Returns the monitor the cursor is currently on, or nullptr if none.
static inline PHLMONITOR getCurrentMonitor() {
    return State::monitorState()->query().vec(g_pInputManager->getMouseCoordsInternal()).run();
}

#define GET_CURRENT_MONITOR()         getCurrentMonitor()
#define GET_ACTIVE_WORKSPACE()        GET_CURRENT_MONITOR()->m_activeWorkspace
#define GET_ACTIVE_SPECIALWORKSPACE() GET_CURRENT_MONITOR()->m_activeSpecialWorkspace

// Returns the current active special workspace (if any) or the active 'normal' workspace
static inline PHLWORKSPACE getActiveWorkspace() {
    auto monitor = GET_CURRENT_MONITOR();
    if (!monitor) {
        return nullptr;
    }
    if (monitor->m_activeSpecialWorkspace != nullptr) {
        return monitor->m_activeSpecialWorkspace;
    }
    return monitor->m_activeWorkspace;
}

std::unordered_set<PHLWINDOW> getWindowsOnWorkspace(const uint32_t workspaceId);

/**
 * @brief Parse a string to a valid tag number (1-9) without throwing exceptions.
 *
 * Uses std::from_chars for safe, fast parsing. Returns std::nullopt if:
 * - The string is empty or not a valid integer
 * - The value is outside the valid tag range (1-9)
 *
 * @param str The string to parse
 * @return std::optional<uint16_t> The tag number, or std::nullopt on failure
 */
static inline std::optional<uint16_t> parseTag(std::string_view str) {
    if (str.empty()) {
        return std::nullopt;
    }

    uint16_t value = 0;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);

    // Check for parse errors or trailing characters
    if (ec != std::errc{} || ptr != str.data() + str.size()) {
        return std::nullopt;
    }

    // Validate tag range (1-9)
    if (value < 1 || value > 9) {
        return std::nullopt;
    }

    return value;
}

#endif //HYPRTAGS_UTILS_H
