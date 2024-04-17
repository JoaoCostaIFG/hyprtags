#ifndef HYPRTAGS_TAGSMONITOR_H
#define HYPRTAGS_TAGSMONITOR_H

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "utils.hpp"

class TagsMonitor {
  public:
    TagsMonitor();

    uint16_t getTags() const {
        return this->tags;
    }

    uint16_t getHist() const {
        return this->hist;
    }

    void swapHist() {
        const uint16_t tmp = this->tags;
        this->tags   = this->hist;
        this->hist   = tmp;
    }

    void gotoTag(uint16_t tag);

    bool toogleTag(uint16_t tag, std::vector<CWindow*> borrowedWindows);

    /* Returns true if the tag is the only tag active (current, no borrows) */
    bool        isOnlyTag(uint16_t tag) const;

    static bool isValidTag(uint16_t tag);

  private:
    uint16_t tags;
    uint16_t hist;
    // tag -> vector of WINDOWS
    std::unordered_map<uint16_t, std::vector<CWindow*>> borrowedTags;

    /* Returns true if the tag was activated, false otherwise (was already activate) */
    bool activateTag(uint16_t tag, std::vector<CWindow*> borrowedWindows);

    /* Returns true if the tag was deactivated, false otherwise (was not activate) */
    bool deactivateTag(uint16_t tag);
};

#endif //HYPRTAGS_TAGSMONITOR_H
