// TODO just getting the ID of the monitor might not be enough.
// Need to check that the IDs are constant after plugs/unplugs in the same session
// Can maybe solve by moving the workspaces to a new monitor when the monitors change.
//
// TODO also need to take care of merging workspaces when monitors disappear
//
// TODO Use this to get focus: g_pCompositor->m_pLastWindow
//
// TODO deal with moving windows between monitors:
//     - maybe this could be a handler that checked for window movements and handled them
//     all there.

#define WLR_USE_UNSTABLE

#include <format>
#include <fstream>
#include <unistd.h>
#include <unordered_map>
#include <string>
#include <any>

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/includes.hpp>

#define private public
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/desktop/Workspace.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/helpers/Monitor.hpp>
#include <hyprland/src/desktop/DesktopTypes.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/event/EventBus.hpp>
#undef private

#include "../include/globals.hpp"
#include "../include/utils.hpp"
#include "../include/TagsMonitor.hpp"

// Each monitor has a set of active tags: monitorID -> listOfTags
static std::unordered_map<size_t, TagsMonitor*> g_tagsMonitors;

/**
 * @brief Safely get the TagsMonitor for the current monitor.
 *
 * @return TagsMonitor* or nullptr if monitor not found
 */
static TagsMonitor* getCurrentTagMonitor() {
    auto monitor = GET_CURRENT_MONITOR();
    if (!monitor) {
        return nullptr;
    }

    auto it = g_tagsMonitors.find(monitor->m_id);
    if (it == g_tagsMonitors.end()) {
        return nullptr;
    }

    return it->second;
}

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

static SDispatchResult tagsWorkspace(const std::string& workspace) {
    Log::logger->log(Log::DEBUG, HYPRTAGS ": tags-workspace {}", workspace);

    auto tag = parseTag(workspace);
    if (!tag) {
        return SDispatchResult{.success = false, .error = std::format(HYPRTAGS " tags-workspace: invalid tag '{}'", workspace)};
    }

    auto tagMon = getCurrentTagMonitor();
    if (!tagMon) {
        return SDispatchResult{.success = false, .error = HYPRTAGS " tags-workspace: no monitor found"};
    }

    tagMon->gotoTag(*tag);

    return SDispatchResult{};
}

static SDispatchResult tagsWorkspacealttab(const std::string& ignored) {
    Log::logger->log(Log::DEBUG, HYPRTAGS ": tags-workspacealttab");

    auto tagMon = getCurrentTagMonitor();
    if (!tagMon) {
        return SDispatchResult{.success = false, .error = HYPRTAGS " tags-workspacealttab: no monitor found"};
    }

    tagMon->altTab();

    return SDispatchResult{};
}

static SDispatchResult tagsMovetoworkspacesilent(const std::string& workspace) {
    Log::logger->log(Log::DEBUG, HYPRTAGS ": tags-movetoworkspacesilent {}", workspace);

    auto tagMon = getCurrentTagMonitor();
    if (!tagMon) {
        return SDispatchResult{.success = false, .error = HYPRTAGS " tags-movetoworkspacesilent: no monitor found"};
    }

    if (workspace.rfind("special:", 0) == 0) {
        // special windows in special workspaces are not managed by us. Just need to make sure the window is not borrowed
        // by anyone before moving it.
        Log::logger->log(Log::DEBUG, HYPRTAGS ": tags-movetoworkspacesilent {} is a special workspace", workspace);
        tagMon->unregisterCurrentWindow();
        HyprlandAPI::invokeHyprctlCommand("dispatch", "movetoworkspacesilent " + workspace);
        return SDispatchResult{};
    }

    auto tag = parseTag(workspace);
    if (!tag) {
        return SDispatchResult{.success = false, .error = std::format(HYPRTAGS " tags-movetoworkspacesilent: invalid tag '{}'", workspace)};
    }

    tagMon->moveCurrentWindowToTag(*tag);

    return SDispatchResult{};
}

static SDispatchResult tagsMovetoworkspace(const std::string& workspace) {
    Log::logger->log(Log::DEBUG, HYPRTAGS ": tags-movetoworkspace {}", workspace);

    auto tag = parseTag(workspace);
    if (!tag) {
        return SDispatchResult{.success = false, .error = std::format(HYPRTAGS " tags-movetoworkspace: invalid tag '{}'", workspace)};
    }

    auto tagMon = getCurrentTagMonitor();
    if (!tagMon) {
        return SDispatchResult{.success = false, .error = HYPRTAGS " tags-movetoworkspace: no monitor found"};
    }

    tagMon->moveCurrentWindowToTag(*tag);
    tagMon->gotoTag(*tag);

    return SDispatchResult{};
}

static SDispatchResult tagsToggleworkspace(const std::string& workspace) {
    Log::logger->log(Log::DEBUG, HYPRTAGS ": tags-toggleworkspace {}", workspace);

    auto tag = parseTag(workspace);
    if (!tag) {
        return SDispatchResult{.success = false, .error = std::format(HYPRTAGS " tags-toggleworkspace: invalid tag '{}'", workspace)};
    }

    auto tagMon = getCurrentTagMonitor();
    if (!tagMon) {
        return SDispatchResult{.success = false, .error = HYPRTAGS " tags-toggleworkspace: no monitor found"};
    }

    tagMon->toogleTag(*tag);

    return SDispatchResult{};
}

/**
 * @brief Notification for user-triggered workspace changes that did not go through the plugin.
 *
 * @param workspace The workspace that changed
 */
static void onWorkspace(PHLWORKSPACE workspace) {
    Log::logger->log(Log::DEBUG, HYPRTAGS ": onWorkspace {}", workspace->m_name);

    // if the workspace is special, do nothing (we don't manage special workspaces)
    if (workspace->m_isSpecialWorkspace) {
        return;
    }

    // ignore non-numeric workspace names (named workspaces are not managed by us)
    auto tag = parseTag(workspace->m_name);
    if (!tag) {
        return;
    }

    auto tagMon = getCurrentTagMonitor();
    if (!tagMon) {
        return;
    }

    // if the workspace is the current one, it means we were the ones that triggered the change, so do nothing
    if (tagMon->isOnlyTag(*tag)) {
        return;
    }

    tagMon->gotoTag(*tag);
}

static void onCloseWindow(PHLWINDOW window) {
    Log::logger->log(Log::DEBUG, HYPRTAGS ": onCloseWindow {}", (uintptr_t)(window.get()));

    auto tagMon = getCurrentTagMonitor();
    if (!tagMon) {
        return;
    }
    tagMon->unregisterWindow(window.get());

    // Unregister from all monitors to avoid dangling pointers in borrowedTags
    for (auto& [id, tagMon] : g_tagsMonitors) {
        tagMon->unregisterWindow(window.get());
    }
}

static void onMonitorAdded(PHLMONITOR monitor) {
    const auto monitorId = monitor->m_id;

    Log::logger->log(Log::DEBUG, HYPRTAGS ": onMonitorAdded {}", monitorId);

    if (g_tagsMonitors.find(monitorId) != g_tagsMonitors.end()) {
        Log::logger->log(Log::DEBUG, HYPRTAGS ": onMonitorAdded {} monitor already registered", monitorId);
        return;
    }
    // add new obj
    TagsMonitor* tagsMonitor  = new TagsMonitor(monitorId);
    g_tagsMonitors[monitorId] = tagsMonitor;
}

static void onMonitorRemoved(PHLMONITOR monitor) {
    const auto monitorId = monitor->m_id;

    Log::logger->log(Log::DEBUG, HYPRTAGS ": onMonitorRemoved {}", monitorId);

    if (g_tagsMonitors.find(monitorId) == g_tagsMonitors.end()) {
        Log::logger->log(Log::DEBUG, HYPRTAGS ": onMonitorRemoved {} monitor not registered", monitorId);
        return;
    }
    // remove monitor
    delete g_tagsMonitors[monitorId];
    g_tagsMonitors.erase(monitorId);
}

static void onMoveWindow(PHLWINDOW window, PHLWORKSPACE workspace) {
    if (window == nullptr || workspace == nullptr) {
        return;
    }

    Log::logger->log(Log::DEBUG, HYPRTAGS ": onMoveWindow window 0x{:x} to workspace {}", (uintptr_t)(window.get()), workspace->m_id);
}

/**
 * Initialize the monitors.
 * Create a config file to source workspace rules.
 */
static bool generateWorkspaceRulesFile(void) {
    std::ofstream file("/tmp/hyprtags.conf");
    if (!file.is_open()) {
        return false;
    }

    for (const auto& pair : g_tagsMonitors) {
        auto id          = pair.first;
        auto monitorName = g_pCompositor->getMonitorFromID(id)->m_name;
        auto monitor     = pair.second;

        file << std::format("# monitor ", monitorName) << std::endl;
        for (uint32_t i = 1; i < 10; ++i) {
            file << std::format("workspace = {}, defaultName:{}, monitor:{}, persistent:true{}", monitor->getWorkspaceId(i), i, monitorName, ((i == 1) ? ", default:true" : ""))
                 << std::endl;
        }
        file << std::endl;
    }

    file.close();
    return true;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    const std::string HASH        = __hyprland_api_get_hash();
    const std::string CLIENT_HASH = __hyprland_api_get_client_hash();

    if (HASH != CLIENT_HASH) {
        HyprlandAPI::addNotification(PHANDLE, HYPRTAGS ": Failure in initialization: Version mismatch (headers ver is not equal to running Hyprland ver)",
                                     CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("[hww] Version mismatch. HASH=" + HASH + ", CLIENT_HASH=" + CLIENT_HASH);
    }

    /* Config */
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprtags:main_display", STRVAL_EMPTY);

    /* Dispatchers */
    bool success = true;
    success      = success && HyprlandAPI::addDispatcherV2(PHANDLE, "tags-workspace", ::tagsWorkspace);
    success      = success && HyprlandAPI::addDispatcherV2(PHANDLE, "tags-workspacealttab", ::tagsWorkspacealttab);
    success      = success && HyprlandAPI::addDispatcherV2(PHANDLE, "tags-movetoworkspacesilent", ::tagsMovetoworkspacesilent);
    success      = success && HyprlandAPI::addDispatcherV2(PHANDLE, "tags-movetoworkspace", ::tagsMovetoworkspace);
    success      = success && HyprlandAPI::addDispatcherV2(PHANDLE, "tags-toggleworkspace", ::tagsToggleworkspace);

    // At the start only the first tag is active
    for (auto& monitor : g_pCompositor->m_monitors) {
        TagsMonitor* tagsMonitor      = new TagsMonitor(monitor->m_id);
        g_tagsMonitors[monitor->m_id] = tagsMonitor;
    }
    if (!generateWorkspaceRulesFile()) {
        Log::logger->log(Log::WARN, HYPRTAGS ": Failed to write workspace rules");
    }

    // so keybinds work and the errors disappear
    // HyprlandAPI::reloadConfig();
    // skipping api to get instant reload
    g_pConfigManager->reload();

    static auto* const MAIN_DISPLAY = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprtags:main_display")->getDataStaticPtr();

    static auto        P1 = Event::bus()->m_events.workspace.active.listen([&](PHLWORKSPACE w) { onWorkspace(w); });
    static auto        P2 = Event::bus()->m_events.window.close.listen([&](PHLWINDOW w) { onCloseWindow(w); });
    static auto        P3 = Event::bus()->m_events.monitor.added.listen([&](PHLMONITOR m) { onMonitorAdded(m); });
    static auto        P4 = Event::bus()->m_events.monitor.preRemoved.listen([&](PHLMONITOR m) { onMonitorRemoved(m); });
    static auto        P5 = Event::bus()->m_events.window.moveToWorkspace.listen([&](PHLWINDOW w, PHLWORKSPACE ws) { onMoveWindow(w, ws); });

    // Focus main screen, if configured
    const auto   MAIN_DISPLAY_STR = std::string{*MAIN_DISPLAY};
    TagsMonitor* mainMonitor      = nullptr;
    for (auto& monitor : g_pCompositor->m_monitors) {
        TagsMonitor* tagsMonitor = g_tagsMonitors[monitor->m_id];
        if (monitor->m_name == MAIN_DISPLAY_STR) {
            mainMonitor = tagsMonitor;
            break;
        }
    }
    if (mainMonitor) {
        mainMonitor->gotoTag(1);
    } else {
        Log::logger->log(Log::WARN, HYPRTAGS ": Failed to find configured main display");
    }

    HyprlandAPI::addNotification(PHANDLE, HYPRTAGS ": Initialized successfully!", CHyprColor{0.2, 1.0, 0.2, 1.0}, 5000);
    return {HYPRTAGS, "Hyprland version of DWM's tag system", "JoaoCostaIFG", "1.2"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    HyprlandAPI::addNotification(PHANDLE, HYPRTAGS ": Unloaded!", CHyprColor{0.2, 1.0, 0.2, 1.0}, 5000);

    for (auto& [id, monitor] : g_tagsMonitors) {
        delete monitor;
    }
    g_tagsMonitors.clear();
}
