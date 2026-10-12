# SCWiki C++ Rewrite Plan

## Overview

This document provides a comprehensive plan and implementation stubs for rewriting the SCWiki Go application in C++.

**SCWiki** is a terminal-based Star Citizen wiki/tracker for managing materials, blueprints, inventory, and crafting plans. This rewrite maintains all existing functionality while leveraging C++'s performance, type safety, and native compilation advantages.

---

## Project Structure

```
scwiki/
├── CMakeLists.txt                    # Main build configuration
├── include/
│   └── scwiki/                       # Public headers
│       ├── app.hpp                   # Application class
│       ├── data.hpp                  # Data model classes
│       ├── terminal.hpp              # Terminal I/O abstraction
│       └── utils.hpp                 # Utility functions
└── src/
    ├── main.cpp                      # Entry point
    ├── app.cpp                       # Application implementation
    ├── data.cpp                      # Data model with JSON serialization
    ├── utils.cpp                     # Utility functions
    ├── terminal_unix.cpp             # Unix terminal implementation
    └── terminal_windows.cpp          # Windows terminal implementation
```

---

## Key Components

### 1. Data Model (`data.hpp` / `data.cpp`)

- **Location**: Mining location with name and spawn percentage
- **Material**: Star Citizen material with RS signature, tier, kind, locations
- **Inventory**: Player's material stock with quantity and quality
- **BlueprintMaterial**: Material requirement for blueprints with min quality
- **Blueprint**: Crafting recipe with materials, output, quantity
- **PlanItem**: Blueprint in crafting plan with craft count
- **Data**: Complete application state

**Features**:
- Full JSON serialization using nlohmann/json
- Data migration support (version 1 -> 2)
- Seed materials with default game data
- Compatible with existing Go-created JSON files

### 2. Terminal Handling (`terminal.hpp` / `terminal_unix.cpp` / `terminal_windows.cpp`)

**Cross-platform abstraction**:
- Raw mode terminal control
- Single key reading
- Line input with prompts
- Screen clearing (ANSI escape sequences)
- Terminal size detection

**Platform support**:
- Unix (Linux/macOS): Uses termios.h and sys/ioctl.h
- Windows: Uses Windows API and conio.h

### 3. Application Core (`app.hpp` / `app.cpp`)

**Main App class** handles:
- Application lifecycle (load, run, save)
- Screen navigation (5 screens)
- Keyboard input handling
- All user actions
- Screen rendering

**Screens**:
- Dashboard: Overview of tracked materials and blueprints
- Materials: Browse and manage materials and inventory
- Blueprints: Browse recipes and material requirements
- Scanner: RS signature lookup
- Planner: Crafting plan with material roll-up

### 4. Utilities (`utils.hpp` / `utils.cpp`)

- String manipulation (lowercase, trim, case-insensitive contains)
- Math utilities (clamp, min, max)
- Window/pagination calculation
- Top locations formatting

---

## Build Instructions

### Prerequisites

- C++20 compiler (GCC 10+, Clang 10+, MSVC 19.29+)
- CMake 3.15+
- nlohmann/json library (automatically fetched by CMake)

### Build Steps

```bash
# Create build directory
mkdir build
cd build

# Configure and build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .

# Run
./scwiki

# Or install
cmake --install .
scwiki
```

### Windows (PowerShell)

```powershell
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 17 2022"
cmake --build . --config Release
.\Release\scwiki.exe
```

---

## Implementation Status

The following files have been created with full implementations:

### Headers (include/scwiki/)
- [x] `data.hpp` - Data model declarations
- [x] `terminal.hpp` - Terminal abstraction interface
- [x] `app.hpp` - Application class interface
- [x] `utils.hpp` - Utility function declarations

### Source Files (src/)
- [x] `main.cpp` - Entry point
- [ ] `data.cpp` - Data model implementations (stub created)
- [ ] `terminal_unix.cpp` - Unix terminal (stub created)
- [ ] `terminal_windows.cpp` - Windows terminal (stub created)
- [ ] `app.cpp` - Application implementation (stub created)
- [ ] `utils.cpp` - Utility implementations (stub created)

---

## Next Steps

To complete the rewrite:

1. **Implement data.cpp**: JSON serialization methods for all data structures
2. **Implement terminal_unix.cpp**: Unix terminal control using termios
3. **Implement terminal_windows.cpp**: Windows terminal control using API
4. **Implement app.cpp**: Complete application logic and rendering
5. **Implement utils.cpp**: Utility functions
6. **Test**: Verify on all target platforms
7. **Verify**: Ensure data compatibility with Go version

---

## Key Design Decisions

### 1. Modern C++ (C++20)
- Use of `std::optional` for nullable fields
- Structured bindings
- Range-based for loops
- Smart pointers where appropriate

### 2. RAII Principle
- All resources managed by constructors/destructors
- Terminal initialization/restoration automatic
- Exception-safe resource management

### 3. Cross-Platform Support
- Platform detection via preprocessor (`SCWIKI_WINDOWS` / `SCWIKI_UNIX`)
- Separate terminal implementations
- Standard C++17 filesystem for path handling

### 4. JSON Serialization
- nlohmann/json library (header-only, well-tested)
- Custom serialization methods for each data type
- Full compatibility with Go JSON structure

### 5. Error Handling
- Exceptions for error conditions
- Cleanup in destructors
- User-friendly error messages

---

## Feature Parity Checklist

### Core Features
- [ ] Material catalog with 29 default materials
- [ ] RS signature tracking
- [ ] Location metadata per material
- [ ] Material tier and kind classification
- [ ] Inventory tracking (quantity + quality)
- [ ] Blueprint definitions
- [ ] Blueprint material requirements
- [ ] Minimum quality thresholds
- [ ] Crafting plan persistence

### User Interface
- [ ] 5 main screens with navigation
- [ ] Keyboard shortcuts (1-5, j/k, g/G, /, t, a, x, s, c, q)
- [ ] Filtering capability
- [ ] Status messages
- [ ] Material roll-up in planner
- [ ] Deficit and quality status indicators

### Data Management
- [ ] JSON data persistence
- [ ] Atomic writes (tmp file + rename)
- [ ] Default data directory (~/.local/share/scwiki/)
- [ ] Environment variable override (SCWIKI_DATA)
- [ ] Data migration (version 1 -> 2)
- [ ] Seed materials on first run

### Terminal Features
- [ ] Raw mode terminal control
- [ ] Single key reading
- [ ] Line input prompts
- [ ] ANSI escape sequences for screen clearing
- [ ] Terminal size detection

---

## Timeline Estimate

| Phase | Duration | Description |
|-------|----------|-------------|
| 1. Foundation | 1-2 days | Project setup, data model, JSON serialization |
| 2. Terminal | 1-2 days | Cross-platform terminal control |
| 3. Core App | 2-3 days | Application logic, navigation, input |
| 4. Screens | 3-4 days | Implement all 5 screens with full functionality |
| 5. Testing | 1-2 days | Cross-platform verification, bug fixes |
| **Total** | **1-2 weeks** | Complete rewrite |

---

## Files Created

The following stub files have been created in `cpp_stubs/` directory:

### Build System
- `CMakeLists.txt` - Complete CMake configuration with JSON library fetch

### Headers
- `include/scwiki/data.hpp` - Complete data model with all structs
- `include/scwiki/terminal.hpp` - Terminal abstraction interface
- `include/scwiki/app.hpp` - Application class with all methods
- `include/scwiki/utils.hpp` - Utility function declarations

### Source Files
- `src/main.cpp` - Complete entry point with error handling

---

## Quick Start

To begin the rewrite:

1. Copy the `cpp_stubs` directory to your C++ project location
2. Implement the missing source files (data.cpp, terminal_*.cpp, app.cpp, utils.cpp)
3. Build with CMake
4. Test on your target platforms

---

## Notes

- The C++ version should be **fully compatible** with existing Go-created `wiki.json` files
- All keyboard shortcuts and behavior should match the Go version
- The UI layout should be as close as possible to the original
- Performance should be at least as good as the Go version

---

*Plan created: 2026-10-12*
*Status: Implementation stubs ready*
