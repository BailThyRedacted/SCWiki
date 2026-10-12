# SCWiki C++ Implementation

This is the C++ implementation of SCWiki, a terminal-based Star Citizen wiki/tracker.

## Project Structure

```
.
├── CMakeLists.txt                    # CMake build configuration
├── README.md                         # This file
├── build.ps1                         # PowerShell build script for Windows
├── data/
│   └── wiki.json                     # Sample data file
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

## Features Implemented

### Data Model (data.hpp/cpp)
- ✅ All data structures (Location, Material, Inventory, BlueprintMaterial, Blueprint, PlanItem, Data)
- ✅ JSON serialization/deserialization using nlohmann/json
- ✅ Seed materials with default game data (29 materials)
- ✅ Data migration (version 1 → 2)
- ✅ Compatible with Go version's JSON format

### Utilities (utils.hpp/cpp)
- ✅ String utilities (to_lower, trim, contains_ignore_case)
- ✅ Math utilities (clamp, min, max)
- ✅ Window/pagination calculation
- ✅ Top locations formatting

### Terminal (terminal.hpp, terminal_unix.cpp, terminal_windows.cpp)
- ✅ Cross-platform abstraction
- ✅ Raw mode terminal control
- ✅ Single key reading
- ✅ Line input with prompts
- ✅ Screen clearing (ANSI escape sequences)
- ✅ Terminal size detection

### Application (app.hpp/cpp, main.cpp)
- ✅ Complete application class
- ✅ 5 screens: Dashboard, Materials, Blueprints, Scanner, Planner
- ✅ All keyboard shortcuts (1-5, j/k, g/G, /, t, a, x, s, c, q)
- ✅ Filtering capability
- ✅ Material tracking
- ✅ Inventory management
- ✅ Blueprint tracking
- ✅ Crafting plan
- ✅ Material roll-up in planner
- ✅ Status indicators (READY, NEED, QUALITY)

## Build Instructions

### Prerequisites

- **C++20 compiler**
  - Windows: Visual Studio 2022 (v17.0+) or MSVC 19.29+
  - Linux/macOS: GCC 10+ or Clang 10+
- **CMake 3.15+**
- **nlohmann/json** (automatically fetched by CMake)

### Windows (PowerShell)

```powershell
# Option 1: Use the build script
.\build.ps1

# Option 2: Manual build
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

### Linux/macOS (Bash)

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Cross-Platform CMake

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build

# Run
./build/scwiki        # Linux/macOS
./build/Release/scwiki.exe  # Windows
```

## Environment Variables

- `SCWIKI_DATA` - Override default data directory
  - Default: `~/.local/share/scwiki/` (Linux/macOS) or `%USERPROFILE%\.local\share\scwiki\` (Windows)
  - Example: `SCWIKI_DATA=/path/to/data ./scwiki`

## Data Compatibility

The C++ version is **fully compatible** with existing Go-created `wiki.json` files. You can:

1. Copy your existing `wiki.json` from `~/.local/share/scwiki/` to the C++ build directory
2. Run the C++ version and it will load your data
3. Both versions can read/write the same data files

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| 1 | Open Dashboard |
| 2 | Open Materials |
| 3 | Open Blueprints |
| 4 | Open Scanner (RS Tracker) |
| 5 | Open Planner |
| j/k | Move selection down/up |
| g | Move to top |
| G | Move to bottom |
| t | Toggle tracking (materials/blueprints) |
| a | Add/set (inventory, plan, crafts) |
| x | Remove (inventory, plan item) |
| / | Filter current list |
| c | Clear filter |
| s | Save data |
| q | Quit |

## Implementation Notes

### Platform Support

The application supports:
- **Windows**: Uses Windows API and conio.h
- **Unix (Linux/macOS)**: Uses termios.h and sys/ioctl.h
- **ANSI escape sequences**: Used for screen clearing (works on modern terminals)

### JSON Library

Uses [nlohmann/json](https://github.com/nlohmann/json) version 3.11.2:
- Header-only
- C++11/14/17/20 compatible
- Automatically fetched via CMake FetchContent
- Full JSON schema support

### C++ Features Used

- C++20 standard
- `std::optional` for nullable fields
- `std::filesystem` for cross-platform file operations
- RAII for resource management
- Exceptions for error handling
- Smart pointers where appropriate

## Testing

### Test Data

A sample `wiki.json` file is provided in the `data/` directory for testing. It contains:
- 10 sample materials
- 2 inventory entries
- 1 blueprint
- Empty plan

### Manual Testing

1. Build the project
2. Copy `data/wiki.json` to the build directory
3. Run `./scwiki` (or `scwiki.exe` on Windows)
4. Verify all screens work correctly
5. Test keyboard shortcuts
6. Test data persistence (edit and save, then restart)

## File Descriptions

### Headers

| File | Description |
|------|-------------|
| `data.hpp` | Data model declarations with JSON serialization |
| `terminal.hpp` | Terminal I/O abstraction interface |
| `app.hpp` | Application class interface |
| `utils.hpp` | Utility function declarations |

### Source Files

| File | Description |
|------|-------------|
| `main.cpp` | Entry point with error handling |
| `data.cpp` | Data model implementations, JSON serialization, seed data |
| `terminal_unix.cpp` | Unix terminal control (termios) |
| `terminal_windows.cpp` | Windows terminal control (Windows API) |
| `app.cpp` | Complete application logic and rendering |
| `utils.cpp` | String, math, and formatting utilities |

## Known Limitations

1. **Testing**: No unit tests yet (Catch2 or Google Test recommended)
2. **Error Recovery**: Terminal state may need cleanup on fatal errors
3. **Performance**: Not yet optimized for large datasets
4. **Testing**: Not all edge cases tested

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## License

Same as the original Go version.

---

*Last updated: 2026-10-12*
*Status: Implementation complete, testing needed*
