// TODO just getting the ID of the monitor might not be enough.
// Need to check that the IDs are constant after plugs/unplugs in the same session
// Can maybe solve by moving the workspaces to a new monitor when the monitors change.
//
// TODO also need to take care of merging workspaces when monitors disappear

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
#include "../include/TagsMonitor.hpp"

#define HYPRTAGS "[hyprtags]"

#define GET_CURRENT_MONITOR() g_pCompositor->getMonitorFromCursor()

static const char* SUPERSCRIPT_DIGITS[] = {"⁰", "¹", "²", "³", "⁴", "⁵", "⁶", "⁷", "⁸", "⁹"};

// Each monitor has a set of active tags: monitorID -> listOfTags
static std::unordered_map<size_t, TagsMonitor> g_tagsMonitors;

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

// g_pCompositor->getWindowsOnWorkspace
// g_pCompositor->moveWindowToWorkspaceSafe

static const std::string getWorkspaceName(CMonitor* monitor, const std::string& workspace) {
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

    HyprlandAPI::addNotification(PHANDLE, HYPRTAGS ": Initialized successfully!", CColor{0.2, 1.0, 0.2, 1.0}, 5000);
    return {HYPRTAGS, "Hyprland version of DWM's tag system", "JoaoCostaIFG", "1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    HyprlandAPI::addNotification(PHANDLE, HYPRTAGS ": Unloaded!", CColor{0.2, 1.0, 0.2, 1.0}, 5000);

    g_tagsMonitors.clear();
}
