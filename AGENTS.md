# AGENTS.md - Hyprtags Plugin

This file provides guidance for AI coding agents working on this codebase.

## Project Overview

Hyprtags is a **C++23 Hyprland plugin** that implements DWM-style workspace tags.
It compiles to a shared library (`hyprtags.so`) loaded by the Hyprland compositor.

### Project Structure

```
hyprtags/
├── include/           # Header files (.hpp)
│   ├── globals.hpp    # Global plugin handle (PHANDLE)
│   ├── TagsMonitor.hpp# TagsMonitor class declaration
│   └── utils.hpp      # Utility macros and helpers
├── src/               # Source files (.cpp)
│   ├── main.cpp       # Plugin entry, dispatchers, callbacks
│   ├── TagsMonitor.cpp# TagsMonitor implementation
│   └── utils.cpp      # Utility implementations
├── .clang-format      # Code formatting rules
├── Makefile           # Primary build system
├── meson.build        # Alternative Meson build
├── hyprpm.toml        # Hyprland plugin manager config
└── default.nix        # Nix package definition
```

## Build Commands

```bash
# Build the plugin (produces hyprtags.so)
make

# Clean build artifacts
make clean

# Install via hyprpm (Hyprland Plugin Manager)
hyprpm add https://github.com/JoaoCostaIFG/hyprtags
hyprpm enable hyprtags
```

### Build Requirements

- `g++` with C++23 support (`-std=c++2b`)
- Dependencies: `hyprland`, `pixman-1`, `libdrm`
- pkg-config for dependency resolution

## Testing

No formal testing framework is configured. To test changes:

1. Build with `make`
2. Load plugin in Hyprland: `hyprctl plugin load $(pwd)/hyprtags.so`
3. Unload plugin in Hyprland: `hyprctl plugin unload $(pwd)/hyprtags.so`
4. Test dispatchers manually: `hyprctl dispatch tags-workspace 2`
5. Check logs: `journalctl --user -u hyprland` or Hyprland log file

## Code Formatting

Format code using clang-format before committing:

```bash
clang-format -i src/*.cpp include/*.hpp
```

### Key Formatting Rules (.clang-format)

| Setting | Value |
|---------|-------|
| Base Style | LLVM |
| Indent Width | 4 spaces |
| Tab Width | 4 (tabs never used) |
| Column Limit | 180 characters |
| Pointer Alignment | Left (`int* p`) |
| Brace Style | Attach (same line) |
| Short If Statements | Never on single line |
| Align Assignments | Yes |
| Align Declarations | Across empty lines |
| Namespace Indentation | All |
| Sort Includes | No (preserve order) |

## Code Style Guidelines

### Include Order

Maintain this order (do not let clang-format sort):

```cpp
// 1. System/STL headers
#include <cstdint>
#include <format>
#include <string>
#include <unordered_map>

// 2. External dependencies (Hyprland)
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/includes.hpp>

// 3. Private access hack (when needed)
#define private public
#include <hyprland/src/config/ConfigManager.hpp>
#undef private

// 4. Local headers (relative paths)
#include "../include/globals.hpp"
#include "../include/utils.hpp"
```

### Header Guards

Use traditional include guards (preferred in this codebase):

```cpp
#ifndef HYPRTAGS_FILENAME_H
#define HYPRTAGS_FILENAME_H
// ...
#endif //HYPRTAGS_FILENAME_H
```

### Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| Classes | PascalCase | `TagsMonitor` |
| Member functions | camelCase | `gotoTag()`, `isValidTag()` |
| Member variables | camelCase | `monitorId`, `borrowedTags` |
| Local variables | camelCase | `workspaceIdx`, `tagMon` |
| Static functions | camelCase | `tagsWorkspace()`, `onWorkspace()` |
| Global variables | `g_` prefix | `g_tagsMonitors`, `g_pCompositor` |
| Plugin handle | PHANDLE | `PHANDLE` (global) |
| Macros/Constants | UPPER_SNAKE | `HYPRTAGS`, `TAG2BIT`, `WLR_USE_UNSTABLE` |
| Callback params | camelCase | `self`, `data`, `info` |

### Type Usage

- Use fixed-width integers: `uint16_t`, `uint32_t`, `uint64_t`
- Use Hyprland smart pointer types: `PHLWINDOW`, `PHLWORKSPACE`, `PHLMONITOR`
- Prefer STL containers: `std::unordered_map`, `std::unordered_set`
- Use C++20/23 features: `std::format`, `std::any`, `std::any_cast`

### Error Handling

Return `SDispatchResult` from dispatcher functions:

```cpp
// Error case
if (!TagsMonitor::isValidTag(workspaceIdx)) {
    return SDispatchResult{.success = false, .error = std::format(HYPRTAGS ": message {}", workspace)};
}

// Success case (default-constructed = success)
return SDispatchResult{};
```

Use early returns for invalid conditions:

```cpp
if (!isValidTag(tag) || this->isOnlyTag(tag)) {
    return;
}

if (activeWindow == nullptr) {
    return;
}
```

### Logging

Use Hyprland's logging system:

```cpp
Log::logger->log(Log::DEBUG, HYPRTAGS ": tags-workspace {}", workspace);
Log::logger->log(Log::WARN, HYPRTAGS ": Failed to write workspace rules");
```

Always prefix log messages with `HYPRTAGS` macro.

### Macros

Define utility macros in `include/utils.hpp`:

```cpp
#define HYPRTAGS "[hyprtags]"
#define TAG2BIT(tag) (1 << (tag - 1))
#define GET_CURRENT_MONITOR() g_pCompositor->getMonitorFromCursor()
```

### Class Structure

```cpp
class TagsMonitor {
  public:
    // Constructor first
    TagsMonitor(uint64_t monitorId);
    
    // Public getters
    uint16_t getTags() const;
    
    // Public methods
    void gotoTag(uint16_t tag);
    
    // Static methods
    static bool isValidTag(uint16_t tag);

  private:
    // Member variables (aligned by clang-format)
    uint64_t monitorId;
    uint16_t tags;
};
```

### C++20/23 Features Used

- Designated initializers: `SDispatchResult{.success = false, .error = "msg"}`
- `std::format` for string formatting
- `std::any` and `std::any_cast` for callback data
- Constructor initializer lists

### Comments

```cpp
// Single line for most comments

/**
 * @brief Doxygen-style for important public functions
 *
 * @param workspace The workspace that changed
 */

// TODO markers for future work
// TODO just getting the ID of the monitor might not be enough.
```

## Hyprland Plugin API Notes

### Required Exports

```cpp
// Version check - DO NOT MODIFY
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

// Plugin initialization
APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle);

// Plugin cleanup
APICALL EXPORT void PLUGIN_EXIT();
```

### Registering Dispatchers

```cpp
HyprlandAPI::addDispatcherV2(PHANDLE, "dispatcher-name", ::dispatcherFunction);
```

### Registering Callbacks

```cpp
static auto P1 = HyprlandAPI::registerCallbackDynamic(
    PHANDLE, "workspace",
    [&](void* self, SCallbackInfo& info, std::any data) { onWorkspace(self, data); }
);
```

### Config Values

```cpp
HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprtags:option_name", STRVAL_EMPTY);
```
