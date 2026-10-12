#pragma once

#include <string>
#include <utility>

namespace scwiki {

// Terminal control class - handles platform-specific terminal operations
class Terminal {
public:
    // Initialize terminal for raw mode
    static void init();
    
    // Restore terminal to normal mode
    static void restore();
    
    // Clear the screen
    static void clear_screen();
    
    // Get terminal dimensions (width, height)
    static std::pair<int, int> get_size();
    
    // Read a single key press
    static char read_key();
    
    // Read a line of input with a prompt
    static std::string line_prompt(const std::string& prompt);
    
    // Check if terminal is in a valid state
    static bool is_initialized();

private:
    // Platform-specific implementations
    #ifdef SCWIKI_WINDOWS
    static void init_windows();
    static void restore_windows();
    static std::pair<int, int> get_size_windows();
    static char read_key_windows();
    static std::string line_prompt_windows(const std::string& prompt);
    #else
    static void init_unix();
    static void restore_unix();
    static std::pair<int, int> get_size_unix();
    static char read_key_unix();
    static std::string line_prompt_unix(const std::string& prompt);
    #endif
};

} // namespace scwiki
