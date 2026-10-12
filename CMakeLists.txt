cmake_minimum_required(VERSION 3.15)
project(SCWiki VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable C++20 features
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

# Link filesystem library
find_package(Filesystem REQUIRED)

# Find or fetch nlohmann/json
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

# Source files
set(SOURCES
    src/main.cpp
    src/app.cpp
    src/data.cpp
    src/utils.cpp
)

set(HEADERS
    include/scwiki/app.hpp
    include/scwiki/data.hpp
    include/scwiki/terminal.hpp
    include/scwiki/utils.hpp
)

# Platform-specific sources
if(WIN32)
    list(APPEND SOURCES src/terminal_windows.cpp)
    target_compile_definitions(scwiki PRIVATE SCWIKI_WINDOWS)
else()
    list(APPEND SOURCES src/terminal_unix.cpp)
    target_compile_definitions(scwiki PRIVATE SCWIKI_UNIX)
endif()

add_executable(scwiki ${SOURCES} ${HEADERS})

target_link_libraries(scwiki PRIVATE nlohmann_json::nlohmann_json stdc++fs)

# Install target
install(TARGETS scwiki DESTINATION bin)
