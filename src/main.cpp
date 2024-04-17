// TODO just getting the ID of the monitor might not be enough.
// Need to check that the IDs are constant after plugs/unplugs in the same session
// Can maybe solve by moving the workspaces to a new monitor when the monitors change.
//
// TODO also need to take care of merging workspaces when monitors disappear
//
// TODO Use this to get focus: g_pCompositor->m_pLastWindow

#include <hyprland/src/desktop/DesktopTypes.hpp>
#define WLR_USE_UNSTABLE

#include <unistd.h>
#include <vector>
#include <unordered_map>
#include <format>
#include <string>

#include <hyprland/src/includes.hpp>
#include <any>

#define private public
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/Window.hpp>
#include <hyprland/src/desktop/Workspace.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/helpers/Monitor.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#undef private

#include "../include/globals.hpp"
#include "../include/utils.hpp"
#include "../include/TagsMonitor.hpp"

// Each monitor has a set of active tags: monitorID -> listOfTags
static std::unordered_map<size_t, TagsMonitor> g_tagsMonitors;

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

void tagsWorkspace(const std::string& workspace) {
    Debug::log(LOG, HYPRTAGS ": tags-workspace {}", workspace);

    uint16_t workspaceIdx = (uint16_t)std::stoi(workspace);
    if (!TagsMonitor::isValidTag(workspaceIdx)) {
        Debug::log(ERR, HYPRTAGS ": tags-workspace {} is invalid", workspace);
        return;
    }

    CMonitor* monitor     = GET_CURRENT_MONITOR();
    auto&     tagsMonitor = g_tagsMonitors[monitor->ID];
    if (tagsMonitor.isOnlyTag(workspaceIdx)) {
        // if we're selecting the current tag, do nothing
        return;
    }

    tagsMonitor.gotoTag(workspaceIdx);
    std::string workspaceName = getWorkspaceName(monitor, workspace);
    HyprlandAPI::invokeHyprctlCommand("dispatch", "workspace name:" + workspaceName);
}

void tagsWorkspacealttab(const std::string& ignored) {
    Debug::log(LOG, HYPRTAGS ": tags-workspacealttab");

    CMonitor* monitor     = GET_CURRENT_MONITOR();
    auto&     tagsMonitor = g_tagsMonitors[monitor->ID];

    // tagsMonitor.swapHist();

    uint16_t tags = tagsMonitor.getHist();
    for (uint16_t tag = 1; tags > 0; ++tag, tags >>= 1) {
        // TODO: for now we're only getting the first active tag
        if (tags & 1) {
            tagsWorkspace(std::to_string(tag));
            return;
        }
    }

    // should be unreachable
    Debug::log(WARN, HYPRTAGS ": tags-workspacealttab found no active tags");
}

void tagsMovetoworkspacesilent(const std::string& workspace) {
    Debug::log(LOG, HYPRTAGS ": tags-movetoworkspacesilent {}", workspace);

    uint16_t workspaceIdx = (uint16_t)std::stoi(workspace);
    if (!TagsMonitor::isValidTag(workspaceIdx)) {
        Debug::log(ERR, HYPRTAGS ": tags-movetoworkspacesilent {} is invalid", workspace);
        return;
    }

    CMonitor*   monitor       = GET_CURRENT_MONITOR();
    std::string workspaceName = getWorkspaceName(monitor, workspace);
    HyprlandAPI::invokeHyprctlCommand("dispatch", "movetoworkspacesilent name:" + workspaceName);
}

void tagsMovetoworkspace(const std::string& workspace) {
    Debug::log(LOG, HYPRTAGS ": tags-movetoworkspace {}", workspace);

    uint16_t workspaceIdx = (uint16_t)std::stoi(workspace);
    if (!TagsMonitor::isValidTag(workspaceIdx)) {
        Debug::log(ERR, HYPRTAGS ": tags-movetoworkspace {} is invalid", workspace);
        return;
    }

    tagsMovetoworkspacesilent(workspace);
    tagsWorkspace(workspace);
}

void tagsToggleworkspace(const std::string& workspace) {
    Debug::log(LOG, HYPRTAGS ": tags-toggleworkspace {}", workspace);

    uint16_t workspaceIdx = (uint16_t)std::stoi(workspace);
    if (!TagsMonitor::isValidTag(workspaceIdx)) {
        Debug::log(ERR, HYPRTAGS ": tags-workspace {} is invalid", workspace);
        return;
    }

    CMonitor* monitor     = GET_CURRENT_MONITOR();
    auto&     tagsMonitor = g_tagsMonitors[monitor->ID];
    if (tagsMonitor.isOnlyTag(workspaceIdx)) {
        // if we're selecting the current tag, do nothing
        Debug::log(WARN, HYPRTAGS ": tags-workspace {} is only tag. Do nothing.", workspace);
        return;
    }

    std::string  borrowedWorkspaceName = getWorkspaceName(monitor, workspace);
    PHLWORKSPACE borrowedWorkspace     = g_pCompositor->getWorkspaceByName(borrowedWorkspaceName);
    auto         borrowedWindows               = getWindowsOnWorkspace(borrowedWorkspace);

    tagsMonitor.toogleTag(workspaceIdx, borrowedWindows);
}

/**
 * @brief Notification for user-triggered workspace changes that did not go through the plugin.
 *
 * @param workspace The workspace that changed
 */
void onWorkspace(std::shared_ptr<CWorkspace> workspace) {
    Debug::log(LOG, HYPRTAGS ": onWorkspace {}", workspace->m_szName);

    // if the workspace is special, do nothing (we don't manage special workspaces)
    if (workspace->m_bIsSpecialWorkspace) {
        return;
    }

    auto&    tagsMonitor  = g_tagsMonitors[workspace->m_iMonitorID];
    uint16_t workspaceIdx = (uint16_t)std::stoi(workspace->m_szName);
    // if the workspace is the current one, it means we were the ones that triggered the change, so do nothing
    if (tagsMonitor.isOnlyTag(workspaceIdx)) {
        return;
    }

    tagsMonitor.gotoTag(workspaceIdx);
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    const std::string HASH = __hyprland_api_get_hash();

    if (HASH != GIT_COMMIT_HASH) {
        HyprlandAPI::addNotification(PHANDLE, HYPRTAGS ": Failure in initialization: Version mismatch (headers ver is not equal to running Hyprland ver)",
                                     CColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("[hww] Version mismatch");
    }

    HyprlandAPI::addDispatcher(PHANDLE, "tags-workspace", tagsWorkspace);
    HyprlandAPI::addDispatcher(PHANDLE, "tags-workspacealttab", tagsWorkspacealttab);
    HyprlandAPI::addDispatcher(PHANDLE, "tags-movetoworkspacesilent", tagsMovetoworkspacesilent);
    HyprlandAPI::addDispatcher(PHANDLE, "tags-movetoworkspace", tagsMovetoworkspace);
    HyprlandAPI::addDispatcher(PHANDLE, "tags-toggleworkspace", tagsToggleworkspace);

    HyprlandAPI::registerCallbackDynamic(PHANDLE, "workspace",
                                         [&](void* self, SCallbackInfo& info, std::any data) { onWorkspace(std::any_cast<std::shared_ptr<CWorkspace>>(data)); });

    // At the start only the first tag is active
    for (auto& monitor : g_pCompositor->m_vMonitors) {
        TagsMonitor tagsMonitor;
        g_tagsMonitors[monitor->ID] = tagsMonitor;

        std::string workspaceName = getWorkspaceName(monitor.get(), "1");
        HyprlandAPI::invokeHyprctlCommand("dispatch", std::format("workspace name:{}", workspaceName));
        HyprlandAPI::invokeHyprctlCommand("dispatch", std::format("moveworkspacetomonitor name:{} {}", workspaceName, monitor->ID));
    }
    // focus main screen
    HyprlandAPI::invokeHyprctlCommand("dispatch", "workspace name:1");

    HyprlandAPI::addNotification(PHANDLE, HYPRTAGS ": Initialized successfully!", CColor{0.2, 1.0, 0.2, 1.0}, 5000);
    return {HYPRTAGS, "Hyprland version of DWM's tag system", "JoaoCostaIFG", "1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    HyprlandAPI::addNotification(PHANDLE, HYPRTAGS ": Unloaded!", CColor{0.2, 1.0, 0.2, 1.0}, 5000);

    g_tagsMonitors.clear();
}
