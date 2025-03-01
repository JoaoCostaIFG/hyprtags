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
#include <unistd.h>
#include <unordered_map>
#include <string>
#include <any>

#include <hyprland/src/includes.hpp>

#include <hyprland/src/Compositor.hpp>

#define private public
#include <hyprland/src/desktop/Window.hpp>
#include <hyprland/src/desktop/Workspace.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/helpers/Monitor.hpp>
#include <hyprland/src/desktop/DesktopTypes.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#undef private

#include "../include/globals.hpp"
#include "../include/utils.hpp"
#include "../include/TagsMonitor.hpp"

#define GET_CURRENT_TAGMONITOR() g_tagsMonitors[GET_CURRENT_MONITOR()->ID]

// Each monitor has a set of active tags: monitorID -> listOfTags
static std::unordered_map<size_t, TagsMonitor*> g_tagsMonitors;

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

static SDispatchResult tagsWorkspace(const std::string& workspace) {
    Debug::log(LOG, HYPRTAGS ": tags-workspace {}", workspace);

    uint16_t workspaceIdx = (uint16_t)std::stoi(workspace);
    if (!TagsMonitor::isValidTag(workspaceIdx)) {
        return SDispatchResult{.success = false, .error = std::format(HYPRTAGS ": tags-workspace {} is invalid", workspace)};
    }

    GET_CURRENT_TAGMONITOR()->gotoTag(workspaceIdx);

    return SDispatchResult{};
}

static SDispatchResult tagsWorkspacealttab(const std::string& ignored) {
    Debug::log(LOG, HYPRTAGS ": tags-workspacealttab");

    GET_CURRENT_TAGMONITOR()->altTab();

    return SDispatchResult{};
}

static SDispatchResult tagsMovetoworkspacesilent(const std::string& workspace) {
    Debug::log(LOG, HYPRTAGS ": tags-movetoworkspacesilent {}", workspace);

    if (workspace.rfind("special:", 0) == 0) {
        // special windows in special workspaces are not managed by us. Just need to make sure the window is not borrowed
        // by anyone before moving it.
        Debug::log(LOG, HYPRTAGS ": tags-movetoworkspacesilent {} is a special workspace", workspace);
        GET_CURRENT_TAGMONITOR()->unregisterCurrentWindow();
        HyprlandAPI::invokeHyprctlCommand("dispatch", "movetoworkspacesilent " + workspace);
        return SDispatchResult{};
    }

    uint16_t workspaceIdx = (uint16_t)std::stoi(workspace);
    if (!TagsMonitor::isValidTag(workspaceIdx)) {
        return SDispatchResult{.success = false, .error = std::format(HYPRTAGS ": tags-movetoworkspacesilent {} is invalid", workspace)};
    }

    GET_CURRENT_TAGMONITOR()->moveCurrentWindowToTag(workspaceIdx);

    return SDispatchResult{};
}

static SDispatchResult tagsMovetoworkspace(const std::string& workspace) {
    Debug::log(LOG, HYPRTAGS ": tags-movetoworkspace {}", workspace);

    uint16_t workspaceIdx = (uint16_t)std::stoi(workspace);
    if (!TagsMonitor::isValidTag(workspaceIdx)) {
        return SDispatchResult{.success = false, .error = std::format(HYPRTAGS ": tags-movetoworkspace {} is invalid", workspace)};
    }

    auto tagMon = GET_CURRENT_TAGMONITOR();
    tagMon->moveCurrentWindowToTag(workspaceIdx);
    tagMon->gotoTag(workspaceIdx);

    return SDispatchResult{};
}

static SDispatchResult tagsToggleworkspace(const std::string& workspace) {
    Debug::log(LOG, HYPRTAGS ": tags-toggleworkspace {}", workspace);

    uint16_t workspaceIdx = (uint16_t)std::stoi(workspace);
    if (!TagsMonitor::isValidTag(workspaceIdx)) {
        return SDispatchResult{.success = false, .error = std::format(HYPRTAGS ": tags-toworkspace {} is invalid", workspace)};
    }

    GET_CURRENT_TAGMONITOR()->toogleTag(workspaceIdx);

    return SDispatchResult{};
}

/**
 * @brief Notification for user-triggered workspace changes that did not go through the plugin.
 *
 * @param workspace The workspace that changed
 */
static void onWorkspace(void* self, std::any data) {
    const auto workspace = std::any_cast<PHLWORKSPACE>(data);
    Debug::log(LOG, HYPRTAGS ": onWorkspace {}", workspace->m_szName);

    // if the workspace is special, do nothing (we don't manage special workspaces)
    if (workspace->m_bIsSpecialWorkspace) {
        return;
    }

    auto     tagMon       = GET_CURRENT_TAGMONITOR();
    uint16_t workspaceIdx = (uint16_t)std::stoi(workspace->m_szName);
    // if the workspace is the current one, it means we were the ones that triggered the change, so do nothing
    if (tagMon->isOnlyTag(workspaceIdx)) {
        return;
    }

    tagMon->gotoTag(workspaceIdx);
}

static void onCloseWindow(void* self, std::any data) {
    // data is guaranteed
    const auto PWINDOW = std::any_cast<PHLWINDOW>(data);

    Debug::log(LOG, HYPRTAGS ": onCloseWindow {}", (uintptr_t)(PWINDOW.get()));

    GET_CURRENT_TAGMONITOR()->unregisterWindow(PWINDOW.get());
}

static void onMonitorAdded(void* self, std::any data) {
    // data is guaranteed
    const auto monitor   = std::any_cast<PHLMONITOR>(data);
    const auto monitorId = monitor->ID;

    Debug::log(LOG, HYPRTAGS ": onMonitorAdded {}", monitorId);

    if (g_tagsMonitors.find(monitorId) != g_tagsMonitors.end()) {
        Debug::log(LOG, HYPRTAGS ": onMonitorAdded {} monitor already registered", monitorId);
        return;
    }
    // add new obj
    TagsMonitor* tagsMonitor  = new TagsMonitor(monitorId);
    g_tagsMonitors[monitorId] = tagsMonitor;
}

static void onMonitorRemoved(void* self, std::any data) {
    // data is guaranteed
    const auto monitor   = std::any_cast<PHLMONITOR>(data);
    const auto monitorId = monitor->ID;

    Debug::log(LOG, HYPRTAGS ": onMonitorRemoved {}", monitorId);

    if (g_tagsMonitors.find(monitorId) == g_tagsMonitors.end()) {
        Debug::log(LOG, HYPRTAGS ": onMonitorRemoved {} monitor not registered", monitorId);
        return;
    }
    // remove monitor
    delete g_tagsMonitors[monitorId];
    g_tagsMonitors.erase(monitorId);
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    const std::string HASH = __hyprland_api_get_hash();

    if (HASH != GIT_COMMIT_HASH) {
        HyprlandAPI::addNotification(PHANDLE, HYPRTAGS ": Failure in initialization: Version mismatch (headers ver is not equal to running Hyprland ver)",
                                     CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("[hww] Version mismatch. HASH=" + HASH + ", GIT_COMMIT_HASH=" + GIT_COMMIT_HASH);
    }

    /* Config */
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprtags:main_display", Hyprlang::STRING{""});
    static auto* const MAIN_DISPLAY = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprtags:main_display")->getDataStaticPtr();

    /* Dispatchers */
    bool success = true;
    success      = success && HyprlandAPI::addDispatcherV2(PHANDLE, "tags-workspace", ::tagsWorkspace);
    success      = success && HyprlandAPI::addDispatcherV2(PHANDLE, "tags-workspacealttab", ::tagsWorkspacealttab);
    success      = success && HyprlandAPI::addDispatcherV2(PHANDLE, "tags-movetoworkspacesilent", ::tagsMovetoworkspacesilent);
    success      = success && HyprlandAPI::addDispatcherV2(PHANDLE, "tags-movetoworkspace", ::tagsMovetoworkspace);
    success      = success && HyprlandAPI::addDispatcherV2(PHANDLE, "tags-toggleworkspace", ::tagsToggleworkspace);
    // so keybinds work and the errors disappear
    HyprlandAPI::reloadConfig();

    // static auto P1 = HyprlandAPI::registerCallbackDynamic(PHANDLE, "workspace", [&](void* self, SCallbackInfo& info, std::any data) { onWorkspace(self, data); });
    static auto P2 = HyprlandAPI::registerCallbackDynamic(PHANDLE, "closeWindow", [&](void* self, SCallbackInfo& info, std::any data) { onCloseWindow(self, data); });
    static auto P3 = HyprlandAPI::registerCallbackDynamic(PHANDLE, "monitorAdded", [&](void* self, SCallbackInfo& info, std::any data) { onMonitorAdded(self, data); });
    static auto P4 = HyprlandAPI::registerCallbackDynamic(PHANDLE, "preMonitorRemoved", [&](void* self, SCallbackInfo& info, std::any data) { onMonitorRemoved(self, data); });

    // At the start only the first tag is active
    for (auto& monitor : g_pCompositor->m_vMonitors) {
        TagsMonitor* tagsMonitor    = new TagsMonitor(monitor->ID);
        g_tagsMonitors[monitor->ID] = tagsMonitor;
    }
    // focus main screen. if configured. Otherwise focus workspace 1 (of monitor 0)
    const auto MAIN_DISPLAY_STR = std::string{*MAIN_DISPLAY};
    bool       found            = false;
    for (auto& monitor : g_pCompositor->m_vMonitors) {
        if (monitor->szName != MAIN_DISPLAY_STR) {
            continue;
        }

        found = true;
        g_tagsMonitors[monitor->ID]->gotoTag(1);
        break;
    }
    if (!found) {
        HyprlandAPI::invokeHyprctlCommand("dispatch", "workspace name:1");
    }

    HyprlandAPI::addNotification(PHANDLE, HYPRTAGS ": Initialized successfully!", CHyprColor{0.2, 1.0, 0.2, 1.0}, 5000);
    return {HYPRTAGS, "Hyprland version of DWM's tag system", "JoaoCostaIFG", "1.2"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    HyprlandAPI::addNotification(PHANDLE, HYPRTAGS ": Unloaded!", CHyprColor{0.2, 1.0, 0.2, 1.0}, 5000);

    g_tagsMonitors.clear();
}
