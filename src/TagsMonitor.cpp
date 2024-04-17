#include "../include/TagsMonitor.hpp"

// tags can only be numbers between 1 and 9 (inclusive)
#define VALIDATE_TAG(tag)                                                                                                                                                          \
    if (tag < 1 || 9 < tag)                                                                                                                                                        \
        return;

#define TAG2BIT(tag) (1 << (tag - 1))

TagsMonitor::TagsMonitor() : tags(1), hist(1) {
    ;
}

void TagsMonitor::gotoTag(uint16_t tag) {
    if (!isValidTag(tag) || this->isOnlyTag(tag)) {
        return;
    }
    this->hist = this->tags;
    this->tags = TAG2BIT(tag);
}

bool TagsMonitor::activateTag(uint16_t tag, std::vector<CWindow*> borrowedWindows) {
    if (!isValidTag(tag)) {
        return false;
    }

    // tag is already active
    if (tags & TAG2BIT(tag)) {
        return false;
    }

    PHLWORKSPACE currentWorkspace = GET_ACTIVE_WORKSPACE();

    // save borrowed windows for this tag
    this->borrowedTags[tag] = borrowedWindows;
    // move them over to our main workspace
    for (auto& w : borrowedWindows) {
        g_pCompositor->moveWindowToWorkspaceSafe(w, currentWorkspace);
    }

    tags |= TAG2BIT(tag);
    return true;
}

bool TagsMonitor::deactivateTag(uint16_t tag) {
    if (!isValidTag(tag)) {
        return false;
    }

    if (!(tags & TAG2BIT(tag))) {
        return false;
    }
    tags &= ~TAG2BIT(tag);
    return true;
}

bool TagsMonitor::toogleTag(uint16_t tag, std::vector<CWindow*> borrowedWindows) {
    if (this->tags & TAG2BIT(tag)) {
        return deactivateTag(tag);
    }
    return activateTag(tag, borrowedWindows);
}

bool TagsMonitor::isOnlyTag(uint16_t tag) const {
    if (!isValidTag(tag)) {
        return false;
    }

    return tags == TAG2BIT(tag);
}

bool TagsMonitor::isValidTag(uint16_t tag) {
    return 1 <= tag && tag <= 9;
}