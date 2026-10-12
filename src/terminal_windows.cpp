#include "scwiki/terminal.hpp"
#include <windows.h>
#include <conio.h>
#include <string>
#include <iostream>
#include <stdexcept>

namespace scwiki {

static HANDLE hStdin = INVALID_HANDLE_VALUE;
static DWORD originalMode = 0;
static bool terminal_initialized = false;

void Terminal::init_windows() {
    if (terminal_initialized) return;

    // Get stdin handle
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    if (hStdin == INVALID_HANDLE_VALUE || hStdin == NULL) {
        throw std::runtime_error("Failed to get stdin handle");
    }

    // Get current mode
    if (!GetConsoleMode(hStdin, &originalMode)) {
        throw std::runtime_error("Failed to get console mode");
    }

    // Set raw mode (disable line input, echo, etc.)
    DWORD mode = ENABLE_EXTENDED_FLAGS;
    if (!SetConsoleMode(hStdin, mode)) {
        throw std::runtime_error("Failed to set console mode");
    }

    // Enable VT100 processing for ANSI escape sequences
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hStdout != INVALID_HANDLE_VALUE && hStdout != NULL) {
        DWORD outMode = 0;
        if (GetConsoleMode(hStdout, &outMode)) {
            outMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hStdout, outMode);
        }
    }

    terminal_initialized = true;
}

void Terminal::restore_windows() {
    if (!terminal_initialized) return;

    // Restore original mode
    if (hStdin != INVALID_HANDLE_VALUE && hStdin != NULL) {
        SetConsoleMode(hStdin, originalMode);
    }

    hStdin = INVALID_HANDLE_VALUE;
    originalMode = 0;
    terminal_initialized = false;
}

std::pair<int, int> Terminal::get_size_windows() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        int width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        int height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        return {width, height};
    }
    return {100, 30};
}

char Terminal::read_key_windows() {
    // Use _getch for single character input
    int ch = _getch();
    return static_cast<char>(ch);
}

std::string Terminal::line_prompt_windows(const std::string& prompt) {
    // Temporarily restore terminal
    restore_windows();
    
    std::cout << "\r" << prompt << std::flush;
    
    std::string result;
    std::getline(std::cin, result);
    
    // Re-init terminal
    init_windows();
    
    return scwiki::trim(result);
}

// Initialize terminal (Windows version)
void Terminal::init() {
    #ifdef SCWIKI_WINDOWS
    init_windows();
    #endif
}

// Restore terminal (Windows version)
void Terminal::restore() {
    #ifdef SCWIKI_WINDOWS
    restore_windows();
    #endif
}

// Clear screen
void Terminal::clear_screen() {
    std::cout << "\x1b[2J\x1b[H" << std::flush;
}

// Get terminal size
std::pair<int, int> Terminal::get_size() {
    #ifdef SCWIKI_WINDOWS
    return get_size_windows();
    #else
    return {100, 30};
    #endif
}

// Read a single key
char Terminal::read_key() {
    #ifdef SCWIKI_WINDOWS
    return read_key_windows();
    #else
    return 0;
    #endif
}

// Read a line with prompt
std::string Terminal::line_prompt(const std::string& prompt) {
    #ifdef SCWIKI_WINDOWS
    return line_prompt_windows(prompt);
    #else
    return "";
    #endif
}

// Check if initialized
bool Terminal::is_initialized() {
    return terminal_initialized;
}

} // namespace scwiki
