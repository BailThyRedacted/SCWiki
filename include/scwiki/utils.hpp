#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cmath>

namespace scwiki {

// Forward declaration
struct Location;

// String utilities
std::string to_lower(const std::string& str);
std::string trim(const std::string& str);
bool contains_ignore_case(const std::string& haystack, const std::string& needle);

// Math utilities
double clamp(double value, double min, double max);
int min(int a, int b);
int max(int a, int b);

// Window calculation for pagination
std::pair<int, int> window(int selected, int count, int rows);

// Selection marker
std::string selmark(int index, int selected);

// Format top locations for display
std::string format_top_locations(const std::vector<Location>& locations);

} // namespace scwiki
