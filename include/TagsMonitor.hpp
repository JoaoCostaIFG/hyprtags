#ifndef HYPRTAGS_TAGSMONITOR_H
#define HYPRTAGS_TAGSMONITOR_H

#include <unordered_set>

class TagsMonitor {
  public:
    std::unordered_set<int> tags;
    std::unordered_set<int> hist;
};

#endif //HYPRTAGS_TAGSMONITOR_H
