#ifndef HYPRTAGS_TAGSMONITOR_H
#define HYPRTAGS_TAGSMONITOR_H

#include <cstdint>
#include <unordered_map>
#include <unordered_set>

#include "utils.hpp"

class TagsMonitor {
  public:
    TagsMonitor(uint64_t monitorId);

    uint16_t getTags() const {
        return this->tags;
    }

    uint16_t getHist() const {
        return this->hist;
    }

    void swapHist() {
        const uint16_t tmp = this->tags;
        this->tags         = this->hist;
        this->hist         = tmp;
    }

    void gotoTag(uint16_t tag);

    bool toogleTag(uint16_t tag);

    void moveCurrentWindowToTag(uint16_t tag);

    void altTab();

    /* Returns true if the tag is the only tag active (current, no borrows) */
    bool        isOnlyTag(uint16_t tag) const;

    /* garantees that the current window is not registered as borrowed by anyone.
     * This is useful when the window is moved to a special workspace
     */
    void        unregisterCurrentWindow();

    static bool isValidTag(uint16_t tag);

  private:
    uint64_t                                                   monitorId;
    std::string                                                identifier;

    uint16_t                                                   tags;
    uint16_t                                                   mainTag;
    std::unordered_map<uint16_t, std::unordered_set<CWindow*>> borrowedTags; // tag -> set of WINDOWS
    uint16_t                                                   hist;
    uint16_t                                                   histMainTag;

    /* Returns true if the tag was activated, false otherwise (was already activate) */
    bool activateTag(uint16_t tag);

    /* Returns true if the tag was deactivated, false otherwise (was not activate) */
    bool        deactivateTag(uint16_t tag);

    void        setMonitorIdentifier(uint64_t monitorId);

    std::string getWorkspaceName(uint16_t tag) const;
};

#endif //HYPRTAGS_TAGSMONITOR_H
