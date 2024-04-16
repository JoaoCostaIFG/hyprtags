// TODO just getting the ID of the monitor might not be enough.
// Need to check that the IDs are constant after plugs/unplugs in the same session
// Can maybe solve by moving the workspaces to a new monitor when the monitors change.
//
// TODO also need to take care of merging workspaces when monitors disappear

#define WLR_USE_UNSTABLE

#include <unistd.h>
#include <vector>
#include <unordered_set>
#include <map>
#include <format>

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
static std::map<size_t, TagsMonitor> g_tagsMonitors;

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

    CMonitor* monitor = GET_CURRENT_MONITOR();

    int       workspaceIdx = std::stoi(workspace);
    auto&     tags         = g_tagsMonitors[monitor->ID].tags;
    if (tags.size() == 1 && tags.contains(workspaceIdx)) {
        // if we're selecting the current tag, do nothing
        return;
    }

    // we're selecting a new tag, update history
    g_tagsMonitors[monitor->ID].hist = tags;
    g_tagsMonitors[monitor->ID].tags.clear();
    g_tagsMonitors[monitor->ID].tags.insert(workspaceIdx);

    std::string workspaceName = getWorkspaceName(monitor, workspace);

    HyprlandAPI::invokeHyprctlCommand("dispatch", "workspace name:" + workspaceName);
}

void tagsWorkspacealttab(const std::string& ignored) {
    Debug::log(LOG, HYPRTAGS ": tags-workspacealttab");

    CMonitor* monitor = GET_CURRENT_MONITOR();

    // switch history
    // auto prevTags = g_monitorTagsHist[monitor->ID];
    // g_monitorTagsHist[monitor->ID] = g_monitorTags[monitor->ID];
    // g_monitorTags[monitor->ID] = prevTags;

    tagsWorkspace(std::to_string(*g_tagsMonitors[monitor->ID].hist.begin()));
}

void tagsMovetoworkspacesilent(const std::string& workspace) {
    Debug::log(LOG, HYPRTAGS ": tags-movetoworkspacesilent {}", workspace);

    CMonitor*   monitor       = GET_CURRENT_MONITOR();
    std::string workspaceName = getWorkspaceName(monitor, workspace);

    HyprlandAPI::invokeHyprctlCommand("dispatch", "movetoworkspacesilent name:" + workspaceName);
}

void tagsMovetoworkspace(const std::string& workspace) {
    Debug::log(LOG, HYPRTAGS ": tags-movetoworkspace {}", workspace);
    tagsMovetoworkspacesilent(workspace);
    tagsWorkspace(workspace);
}

/**
 * @brief Notification for user-triggered workspace changes that did not go through the plugin.
 * 
 * @param workspace The workspace that changed
 */
void onWorkspace(CWorkspace* workspace) {
    Debug::log(LOG, HYPRTAGS ": onWorkspace {}", workspace->m_szName);

    // if the workspace is special, do nothing (we don't manage special workspaces)
    if (workspace->m_bIsSpecialWorkspace) {
        return;
    }

    CMonitor* monitor = GET_CURRENT_MONITOR();

    // update history
    g_tagsMonitors[monitor->ID].hist = g_tagsMonitors[monitor->ID].tags;
    g_tagsMonitors[monitor->ID].tags.clear();
    g_tagsMonitors[monitor->ID].tags.insert(std::stoi(workspace->m_szName));
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

    // HyprlandAPI::registerCallbackDynamic(PHANDLE, "workspace", [&](void* self, SCallbackInfo& info, std::any data) { onWorkspace(std::any_cast<CWorkspace*>(data)); });

    // At the start only the first tag is active
    for (auto& monitor : g_pCompositor->m_vMonitors) {
        g_tagsMonitors[monitor->ID].tags.insert(1);
        g_tagsMonitors[monitor->ID].hist.insert(1);

        std::string workspaceName = getWorkspaceName(monitor.get(), "1");
        HyprlandAPI::invokeHyprctlCommand("dispatch", std::format("workspace name:{}", workspaceName));
        HyprlandAPI::invokeHyprctlCommand("dispatch", std::format("moveworkspacetomonitor name:{} {}", workspaceName, std::to_string(monitor->ID)));
    }

    HyprlandAPI::addNotification(PHANDLE, HYPRTAGS ": Initialized successfully!", CColor{0.2, 1.0, 0.2, 1.0}, 5000);
    return {HYPRTAGS, "Hyprland version of DWM's tag system", "JoaoCostaIFG", "1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    HyprlandAPI::addNotification(PHANDLE, HYPRTAGS ": Unloaded!", CColor{0.2, 1.0, 0.2, 1.0}, 5000);

    g_tagsMonitors.clear();
}
