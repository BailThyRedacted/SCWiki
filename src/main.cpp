#include "scwiki/app.hpp"
#include "scwiki/terminal.hpp"
#include <iostream>
#include <stdexcept>

int main() {
    try {
        // Get data directory
        std::string data_dir = scwiki::App::default_data_dir();
        scwiki::App app(data_dir);
        
        // Initialize terminal
        scwiki::Terminal::init();
        
        // Run application
        app.run();
        
        // Restore terminal before exit
        scwiki::Terminal::restore();
        
        return 0;
    } catch (const std::exception& e) {
        // Ensure terminal is restored even on error
        scwiki::Terminal::restore();
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
