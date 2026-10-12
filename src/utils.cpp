#include "scwiki/utils.hpp"
#include "scwiki/data.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace scwiki {

// Convert string to lowercase
std::string to_lower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

// Trim whitespace from both ends
std::string trim(const std::string& str) {
    auto start = str.begin();
    while (start != str.end() && std::isspace(static_cast<unsigned char>(*start))) {
        start++;
    }
    auto end = str.end();
    while (end != start && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
        end--;
    }
    return std::string(start, end);
}

// Case-insensitive contains
bool contains_ignore_case(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true;
    std::string lower_haystack = to_lower(haystack);
    std::string lower_needle = to_lower(needle);
    return lower_haystack.find(lower_needle) != std::string::npos;
}

// Clamp value between min and max
double clamp(double value, double min_val, double max_val) {
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

// Minimum of two integers
int min(int a, int b) {
    return (a < b) ? a : b;
}

// Maximum of two integers
int max(int a, int b) {
    return (a > b) ? a : b;
}

// Calculate window for pagination
std::pair<int, int> window(int selected, int count, int rows) {
    if (rows < 5) {
        rows = 5;
    }
    if (count <= rows) {
        return {0, count};
    }
    int start = selected - rows / 2;
    if (start < 0) {
        start = 0;
    }
    if (start + rows > count) {
        start = count - rows;
    }
    return {start, start + rows};
}

// Selection marker for UI
std::string selmark(int index, int selected) {
    return (index == selected) ? ">" : " ";
}

// Format top locations for display
std::string format_top_locations(const std::vector<Location>& locations) {
    if (locations.empty()) {
        return "No location data";
    }

    // Sort by spawn_percent descending, then by name
    std::vector<Location> sorted = locations;
    std::sort(sorted.begin(), sorted.end(), [](const Location& a, const Location& b) {
        // Both have spawn_percent
        if (a.spawn_percent.has_value() && b.spawn_percent.has_value()) {
            if (a.spawn_percent.value() != b.spawn_percent.value()) {
                return a.spawn_percent.value() > b.spawn_percent.value();
            }
        }
        // Only a has spawn_percent
        else if (a.spawn_percent.has_value()) {
            return true;
        }
        // Only b has spawn_percent
        else if (b.spawn_percent.has_value()) {
            return false;
        }
        // Neither has spawn_percent, sort by name
        return a.name < b.name;
    });

    // Take top 3
    if (sorted.size() > 3) {
        sorted.resize(3);
    }

    std::vector<std::string> parts;
    for (const auto& loc : sorted) {
        if (loc.spawn_percent.has_value()) {
            std::ostringstream oss;
            oss << loc.name << " " << static_cast<int>(std::round(loc.spawn_percent.value())) << "%";
            parts.push_back(oss.str());
        } else {
            parts.push_back(loc.name);
        }
    }

    // Join with separator
    if (parts.empty()) {
        return "";
    }
    std::string result = parts[0];
    for (size_t i = 1; i < parts.size(); ++i) {
        result += " • " + parts[i];
    }
    return result;
}

} // namespace scwiki
