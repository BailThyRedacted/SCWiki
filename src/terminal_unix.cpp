#include "scwiki/terminal.hpp"
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <stdexcept>

namespace scwiki {

static struct termios original_termios;
static bool terminal_initialized = false;

void Terminal::init_unix() {
    if (terminal_initialized) return;

    // Get current terminal attributes
    if (tcgetattr(STDIN_FILENO, &original_termios) != 0) {
        throw std::runtime_error("Failed to get terminal attributes");
    }

    // Create raw mode terminal settings
    struct termios raw = original_termios;
    
    // Disable echo, canonical mode, and signals
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    
    // Disable processing
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    
    // Character size (8 bits)
    raw.c_cflag &= ~(CSIZE | PARENB);
    raw.c_cflag |= CS8;
    
    // Minimum characters and timeout
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    // Apply settings
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
        throw std::runtime_error("Failed to set terminal attributes");
    }

    terminal_initialized = true;
}

void Terminal::restore_unix() {
    if (!terminal_initialized) return;

    // Restore original terminal settings
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios) != 0) {
        // Ignore error on restore
    }

    terminal_initialized = false;
}

std::pair<int, int> Terminal::get_size_unix() {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        return {ws.ws_col, ws.ws_row};
    }
    // Default size if we can't get terminal size
    return {100, 30};
}

char Terminal::read_key_unix() {
    char buf = 0;
    ssize_t nread = read(STDIN_FILENO, &buf, 1);
    if (nread == 1) {
        return buf;
    }
    return 0;
}

std::string Terminal::line_prompt_unix(const std::string& prompt) {
    // Temporarily restore terminal to get line input
    restore_unix();
    
    std::cout << "\r" << prompt << std::flush;
    
    std::string result;
    std::getline(std::cin, result);
    
    // Re-init terminal for raw mode
    init_unix();
    
    return scwiki::trim(result);
}

// Initialize terminal (Unix version)
void Terminal::init() {
    #ifdef SCWIKI_UNIX
    init_unix();
    #endif
}

// Restore terminal (Unix version)
void Terminal::restore() {
    #ifdef SCWIKI_UNIX
    restore_unix();
    #endif
}

// Clear screen
void Terminal::clear_screen() {
    std::cout << "\x1b[2J\x1b[H" << std::flush;
}

// Get terminal size
std::pair<int, int> Terminal::get_size() {
    #ifdef SCWIKI_UNIX
    return get_size_unix();
    #else
    return {100, 30}; // Default for non-Unix
    #endif
}

// Read a single key
char Terminal::read_key() {
    #ifdef SCWIKI_UNIX
    return read_key_unix();
    #else
    return 0;
    #endif
}

// Read a line with prompt
std::string Terminal::line_prompt(const std::string& prompt) {
    #ifdef SCWIKI_UNIX
    return line_prompt_unix(prompt);
    #else
    return "";
    #endif
}

// Check if initialized
bool Terminal::is_initialized() {
    return terminal_initialized;
}

} // namespace scwiki
