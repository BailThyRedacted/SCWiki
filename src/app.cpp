#include "scwiki/app.hpp"
#include "scwiki/data.hpp"
#include "scwiki/terminal.hpp"
#include "scwiki/utils.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <algorithm>
#include <stdexcept>

namespace scwiki {

namespace fs = std::filesystem;

// Helper to convert Data to JSON string
std::string data_to_json(const Data& data) {
    nlohmann::json j = data;
    return j.dump(2) + "\n";
}

// Helper to parse JSON string to Data
Data json_to_data(const std::string& json_str) {
    nlohmann::json j = nlohmann::json::parse(json_str);
    return j.get<Data>();
}

// Constructor
App::App(const std::string& data_dir) : data_dir_(data_dir) {
    load();
}

// Destructor
App::~App() {
    // Auto-save on destruction if there are unsaved changes
    // Note: In production, you might want explicit save control
}

// Get default data directory (static method)
std::string App::default_data_dir() {
    // Check environment variable first
    const char* env_data = std::getenv("SCWIKI_DATA");
    if (env_data && std::string(env_data) != "") {
        fs::path path = env_data;
        fs::create_directories(path);
        return path.string();
    }

    // Get home directory
    fs::path home;
    #ifdef _WIN32
    const char* home_env = std::getenv("USERPROFILE");
    if (home_env) {
        home = home_env;
    }
    #else
    const char* home_env = std::getenv("HOME");
    if (home_env) {
        home = home_env;
    }
    #endif

    if (home.empty()) {
        throw std::runtime_error("Cannot determine home directory");
    }

    fs::path data_path = home / ".local" / "share" / "scwiki";
    fs::create_directories(data_path);
    return data_path.string();
}

// Load data from file
void App::load() {
    fs::path data_path = fs::path(data_dir_) / "wiki.json";
    
    if (!fs::exists(data_path)) {
        // Create default data
        data_.version = 2;
        data_.patch = "4.10";
        data_.materials = seed_materials();
        save();
        return;
    }

    // Read file
    std::ifstream file(data_path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open data file: " + data_path.string());
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    try {
        data_ = json_to_data(content);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to parse data file: ") + e.what());
    }

    // Migrate data if needed
    migrate_data(data_);
    
    // Save after migration
    save();
}

// Save data to file
void App::save() {
    fs::path data_path = fs::path(data_dir_) / "wiki.json";
    fs::path temp_path = data_path;
    temp_path += ".tmp";

    try {
        std::string content = data_to_json(data_);

        std::ofstream file(temp_path);
        if (!file.is_open()) {
            message_ = "save failed: cannot open file";
            return;
        }
        file << content;
        file.close();

        // Atomic rename
        fs::rename(temp_path, data_path);
        message_ = "saved";
    } catch (const std::exception& e) {
        message_ = "save failed: " + std::string(e.what());
        // Try to remove temp file if it exists
        if (fs::exists(temp_path)) {
            fs::remove(temp_path);
        }
    }
}

// Main application loop
void App::run() {
    while (!quitting_) {
        render();
        char key = Terminal::read_key();
        if (key != 0) {
            handle_input(key);
        }
    }
}

// Handle keyboard input
void App::handle_input(char key) {
    switch (key) {
        case 'q':
        case 'Q':
            quitting_ = true;
            break;
        case '1':
            open_screen(SCREEN_DASHBOARD);
            break;
        case '2':
            open_screen(SCREEN_MATERIALS);
            break;
        case '3':
            open_screen(SCREEN_BLUEPRINTS);
            break;
        case '4':
            open_screen(SCREEN_SCANNER);
            break;
        case '5':
            open_screen(SCREEN_PLANNER);
            break;
        case 'j':
        case 'J':
            move(1);
            break;
        case 'k':
        case 'K':
            move(-1);
            break;
        case 'g':
            selected_ = 0;
            break;
        case 'G':
            move_to_end();
            break;
        case 't':
        case 'T':
            switch (screen_) {
                case SCREEN_MATERIALS:
                    toggle_material_tracking();
                    break;
                case SCREEN_BLUEPRINTS:
                    toggle_blueprint_tracking();
                    break;
                default:
                    message_ = "tracking is changed from Materials or Blueprints";
                    break;
            }
            break;
        case 'a':
        case 'A':
            switch (screen_) {
                case SCREEN_MATERIALS:
                    add_inventory();
                    break;
                case SCREEN_BLUEPRINTS:
                    toggle_blueprint_plan();
                    break;
                case SCREEN_PLANNER:
                    set_plan_crafts();
                    break;
                default:
                    message_ = "use Materials/Blueprints/Planner for this action";
                    break;
            }
            break;
        case '/':
            prompt_filter();
            break;
        case 's':
        case 'S':
            save();
            break;
        case 'c':
        case 'C':
            filter_.clear();
            selected_ = 0;
            message_ = "filter cleared";
            break;
        case 'x':
        case 'X':
            if (screen_ == SCREEN_PLANNER) {
                remove_plan_item();
            } else if (screen_ == SCREEN_MATERIALS) {
                remove_inventory();
            }
            break;
    }
}

// Open a screen
void App::open_screen(int screen) {
    screen_ = screen;
    selected_ = 0;
    filter_.clear();
    message_.clear();
}

// Move selection
void App::move(int delta) {
    int n = item_count();
    if (n == 0) return;
    
    selected_ += delta;
    if (selected_ < 0) selected_ = 0;
    if (selected_ >= n) selected_ = n - 1;
}

// Move to end
void App::move_to_end() {
    int n = item_count();
    if (n > 0) {
        selected_ = n - 1;
    }
}

// Get item count for current screen
int App::item_count() const {
    switch (screen_) {
        case SCREEN_DASHBOARD:
            return static_cast<int>(tracked_materials().size());
        case SCREEN_MATERIALS:
            return static_cast<int>(filtered_materials().size());
        case SCREEN_BLUEPRINTS:
            return static_cast<int>(filtered_blueprints().size());
        case SCREEN_SCANNER:
            return static_cast<int>(filtered_materials().size());
        case SCREEN_PLANNER:
            return static_cast<int>(filtered_plan_blueprints().size());
        default:
            return 0;
    }
}

// Toggle material tracking
void App::toggle_material_tracking() {
    auto rows = filtered_materials();
    if (selected_ >= static_cast<int>(rows.size())) return;
    
    const std::string& id = rows[selected_].id;
    Material* m = material_by_id(id);
    if (!m) return;
    
    m->tracked = !m->tracked;
    if (m->tracked) {
        message_ = "tracked material: " + m->name;
    } else {
        message_ = "untracked material: " + m->name;
    }
    save();
}

// Toggle blueprint tracking
void App::toggle_blueprint_tracking() {
    auto rows = filtered_blueprints();
    if (selected_ >= static_cast<int>(rows.size())) return;
    
    const std::string& id = rows[selected_].id;
    Blueprint* b = blueprint_by_id(id);
    if (!b) return;
    
    b->tracked = !b->tracked;
    if (b->tracked) {
        message_ = "tracked blueprint: " + b->name;
    } else {
        message_ = "untracked blueprint: " + b->name;
    }
    save();
}

// Add inventory
void App::add_inventory() {
    auto rows = filtered_materials();
    if (selected_ >= static_cast<int>(rows.size())) return;
    
    const Material& m = rows[selected_];
    std::string input = Terminal::line_prompt(m.name + " quantity quality (e.g. 12.5 85): ");
    
    if (input.empty()) return;
    
    // Parse input
    std::istringstream iss(input);
    double quantity, quality;
    
    if (!(iss >> quantity >> quality)) {
        message_ = "expected quantity and quality";
        return;
    }
    
    // Validate
    if (quantity < 0 || quality < 0 || quality > 1000) {
        message_ = "invalid quantity/quality";
        return;
    }
    
    // Update inventory
    Inventory* inv = inventory(m.id);
    if (!inv) {
        data_.inventory.push_back({m.id, quantity, quality});
    } else {
        inv->quantity = quantity;
        inv->quality = quality;
    }
    
    message_ = "inventory updated: " + m.name;
    save();
}

// Remove inventory
void App::remove_inventory() {
    auto rows = filtered_materials();
    if (selected_ >= static_cast<int>(rows.size())) return;
    
    const std::string& id = rows[selected_].id;
    Material* m = material_by_id(id);
    
    // Remove all inventory entries for this material
    data_.inventory.erase(
        std::remove_if(data_.inventory.begin(), data_.inventory.end(),
            [&id](const Inventory& inv) { return inv.material_id == id; }),
        data_.inventory.end()
    );
    
    if (m) {
        message_ = "inventory cleared: " + m->name;
    }
    save();
}

// Prompt for filter
void App::prompt_filter() {
    std::string value = Terminal::line_prompt("filter: ");
    if (!value.empty()) {
        filter_ = trim(value);
        selected_ = 0;
        message_ = "filter: " + filter_;
    }
}

// Toggle blueprint in plan
void App::toggle_blueprint_plan() {
    auto rows = filtered_blueprints();
    if (selected_ >= static_cast<int>(rows.size())) return;
    
    const std::string& id = rows[selected_].id;
    
    // Check if already in plan
    for (auto it = data_.plan.begin(); it != data_.plan.end(); ++it) {
        if (it->blueprint_id == id) {
            data_.plan.erase(it);
            message_ = "removed from plan: " + rows[selected_].name;
            save();
            return;
        }
    }
    
    // Add to plan
    data_.plan.push_back({id, rows[selected_].quantity});
    message_ = "added to plan: " + rows[selected_].name;
    save();
}

// Set plan crafts
void App::set_plan_crafts() {
    auto rows = filtered_plan_blueprints();
    if (selected_ >= static_cast<int>(rows.size())) return;
    
    const std::string& id = rows[selected_].id;
    std::string input = Terminal::line_prompt("output units (0 removes): ");
    
    if (input.empty()) return;
    
    double value;
    std::istringstream iss(input);
    
    if (!(iss >> value) || value < 0) {
        message_ = "invalid craft quantity";
        return;
    }
    
    // Find in plan
    for (auto it = data_.plan.begin(); it != data_.plan.end(); ++it) {
        if (it->blueprint_id == id) {
            if (value == 0) {
                data_.plan.erase(it);
            } else {
                it->crafts = value;
            }
            save();
            return;
        }
    }
    
    // Add if value > 0
    if (value > 0) {
        data_.plan.push_back({id, value});
        save();
    }
}

// Remove plan item
void App::remove_plan_item() {
    auto rows = filtered_plan_blueprints();
    if (selected_ >= static_cast<int>(rows.size())) return;
    
    const std::string& id = rows[selected_].id;
    
    for (auto it = data_.plan.begin(); it != data_.plan.end(); ++it) {
        if (it->blueprint_id == id) {
            data_.plan.erase(it);
            message_ = "removed from plan: " + rows[selected_].name;
            save();
            return;
        }
    }
}

// Render the application
void App::render() {
    Terminal::clear_screen();
    auto [width, height] = Terminal::get_size();
    
    // Header
    std::cout << "SCWIKI • local Star Citizen wiki • patch " << data_.patch << "\n";
    std::cout << "1 Dashboard  2 Materials  3 Blueprints  4 RS Tracker  5 Planner  |  j/k move  g/G top/end\n";
    std::cout << "t track/untrack  a add/set  x remove  / filter  c clear  s save  q quit\n";
    std::cout << std::string(min(width, 120), '-') << "\n";
    
    // Current screen
    switch (screen_) {
        case SCREEN_DASHBOARD:
            render_dashboard(height);
            break;
        case SCREEN_MATERIALS:
            render_materials(height);
            break;
        case SCREEN_BLUEPRINTS:
            render_blueprints(height);
            break;
        case SCREEN_SCANNER:
            render_scanner(height);
            break;
        case SCREEN_PLANNER:
            render_planner(height);
            break;
    }
    
    // Footer
    std::cout << std::string(min(width, 120), '-') << "\n";
    if (!message_.empty()) {
        std::cout << message_ << "\n";
    } else {
        std::cout << "Tip: use t to track materials/blueprints; RS Tracker uses the selected material — no signal search required.\n";
    }
    
    std::cout << std::flush;
}

// Render dashboard
void App::render_dashboard(int height) {
    auto rows = tracked_materials();
    std::cout << "DASHBOARD • " << rows.size() << " tracked materials • " 
              << tracked_blueprint_count() << " tracked blueprints\n\n";
    
    if (rows.empty()) {
        std::cout << "No materials are tracked yet.\n";
        std::cout << "Go to Materials, select a material, and press t.\n";
        return;
    }
    
    // Table header
    std::cout << std::left
              << std::setw(18) << "MATERIAL"
              << std::setw(8) << "RS"
              << std::setw(46) << "TOP 3 LOCATIONS"
              << std::setw(12) << "TRACKED BPs"
              << "\n";
    std::cout << std::string(92, '-') << "\n";
    
    // Calculate window
    auto [start, end] = window(selected_, static_cast<int>(rows.size()), height - 8);
    
    for (int i = start; i < end && i < static_cast<int>(rows.size()); ++i) {
        const Material& m = rows[i];
        std::string locations = format_top_locations(m.locations);
        std::cout << selmark(i, selected_)
                  << std::setw(18) << m.name
                  << std::setw(8) << m.signal_rs
                  << std::setw(46) << locations
                  << std::setw(12) << tracked_blueprint_use_count(m.id)
                  << "\n";
    }
}

// Render materials screen
void App::render_materials(int height) {
    auto rows = filtered_materials();
    std::cout << "MATERIALS • " << rows.size() << " shown\n\n";
    std::cout << std::left
              << std::setw(18) << "NAME"
              << std::setw(5) << "TIER"
              << std::setw(20) << "KIND"
              << std::setw(8) << "RS"
              << std::setw(10) << "TRACKED"
              << std::setw(10) << "OWNED"
              << "\n";
    
    auto [start, end] = window(selected_, static_cast<int>(rows.size()), height - 7);
    
    for (int i = start; i < end && i < static_cast<int>(rows.size()); ++i) {
        const Material& m = rows[i];
        std::string tracked_str = m.tracked ? "yes" : "no";
        std::string owned_str = "-";
        
        if (const Inventory* inv = inventory(m.id)) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << inv->quantity;
            owned_str = oss.str();
        }
        
        std::cout << selmark(i, selected_)
                  << std::setw(18) << m.name
                  << std::setw(5) << m.tier
                  << std::setw(20) << m.kind
                  << std::setw(8) << m.signal_rs
                  << std::setw(10) << tracked_str
                  << std::setw(10) << owned_str
                  << "\n";
    }
    
    std::cout << "\n`t` tracking is independent from inventory; `a` updates owned quantity/quality; `x` clears inventory.\n";
}

// Render blueprints screen
void App::render_blueprints(int height) {
    auto rows = filtered_blueprints();
    std::cout << "BLUEPRINTS • " << rows.size() << " shown\n\n";
    std::cout << std::left
              << std::setw(28) << "NAME"
              << std::setw(18) << "OUTPUT"
              << std::setw(9) << "TRACKED"
              << std::setw(10) << "IN PLAN"
              << "\n";
    
    auto [start, end] = window(selected_, static_cast<int>(rows.size()), height - 15);
    
    for (int i = start; i < end && i < static_cast<int>(rows.size()); ++i) {
        const Blueprint& b = rows[i];
        std::string tracked_str = b.tracked ? "yes" : "no";
        std::string in_plan_str = "no";
        
        PlanItem item = plan_item(b.id);
        if (!item.blueprint_id.empty()) {
            in_plan_str = "yes";
        }
        
        std::cout << selmark(i, selected_)
                  << std::setw(28) << b.name
                  << std::setw(18) << b.output
                  << std::setw(9) << tracked_str
                  << std::setw(10) << in_plan_str
                  << "\n";
    }
    
    if (!rows.empty() && selected_ < static_cast<int>(rows.size())) {
        const Blueprint& b = rows[selected_];
        std::cout << "\nSELECTED • " << b.name << " → " << b.output 
                  << " x" << std::fixed << std::setprecision(2) << b.quantity << "\n";
        std::cout << std::left
                  << std::setw(18) << "MATERIAL"
                  << std::setw(10) << "REQUIRED"
                  << std::setw(10) << "OWNED"
                  << std::setw(10) << "QUALITY"
                  << std::setw(10) << "MIN Q"
                  << "\n";
        
        for (const auto& req : b.materials) {
            const Material* m = material_by_id(req.material_id);
            if (!m) continue;
            
            double owned = 0.0;
            double quality = 0.0;
            if (const Inventory* inv = inventory(req.material_id)) {
                owned = inv->quantity;
                quality = inv->quality;
            }
            
            std::string min_quality_str = "-";
            if (req.min_quality.has_value()) {
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(0) << req.min_quality.value();
                min_quality_str = oss.str();
            }
            
            std::string status = "";
            if (owned + 1e-9 < req.quantity) {
                status = " *NEED*";
            }
            
            std::cout << std::setw(18) << m->name
                      << std::setw(10) << std::fixed << std::setprecision(2) << req.quantity
                      << std::setw(10) << std::fixed << std::setprecision(2) << owned
                      << std::setw(10) << std::fixed << std::setprecision(2) << quality
                      << std::setw(10) << min_quality_str
                      << status
                      << "\n";
        }
        
        std::cout << "\n`t` track this blueprint; `a` add/remove it from the crafting plan.\n";
    }
}

// Render scanner screen
void App::render_scanner(int height) {
    auto rows = filtered_materials();
    std::cout << "RS TRACKER • select a material, then read its signature\n\n";
    std::cout << std::left
              << std::setw(3) << ""
              << std::setw(20) << "MATERIAL"
              << std::setw(8) << "BASE RS"
              << std::setw(5) << "TIER"
              << std::setw(46) << "QUICK INFO"
              << "\n";
    std::cout << std::string(88, '-') << "\n";
    
    auto [start, end] = window(selected_, static_cast<int>(rows.size()), height - 9);
    
    for (int i = start; i < end && i < static_cast<int>(rows.size()); ++i) {
        const Material& m = rows[i];
        std::string locations = format_top_locations(m.locations);
        std::cout << selmark(i, selected_)
                  << std::setw(20) << m.name
                  << std::setw(8) << m.signal_rs
                  << std::setw(5) << m.tier
                  << std::setw(46) << "Top: " + locations
                  << "\n";
    }
    
    if (!rows.empty() && selected_ < static_cast<int>(rows.size())) {
        const Material& m = rows[selected_];
        std::cout << "\nSELECTED MATERIAL: " << m.name
                  << "\nRS SIGNATURE: " << m.signal_rs
                  << "\n\nUse j/k to select the material. `/` still filters material names; there is no RS-value search.\n";
    }
}

// Render planner screen
void App::render_planner(int height) {
    auto rows = filtered_plan_blueprints();
    std::cout << "CRAFTING PLANNER • " << rows.size() << " planned blueprints\n\n";
    std::cout << std::left
              << std::setw(28) << "BLUEPRINT"
              << std::setw(12) << "OUTPUT UNITS"
              << std::setw(10) << "BATCH"
              << "\n";
    
    auto [start, end] = window(selected_, static_cast<int>(rows.size()), height - 15);
    
    for (int i = start; i < end && i < static_cast<int>(rows.size()); ++i) {
        const Blueprint& b = rows[i];
        PlanItem item = plan_item(b.id);
        std::cout << selmark(i, selected_)
                  << std::setw(28) << b.name
                  << std::setw(12) << std::fixed << std::setprecision(2) << item.crafts
                  << std::setw(10) << std::fixed << std::setprecision(2) << b.quantity
                  << "\n";
    }
    
    // Material roll-up
    std::cout << "\nMATERIAL ROLL-UP\n";
    std::cout << std::left
              << std::setw(18) << "MATERIAL"
              << std::setw(10) << "REQUIRED"
              << std::setw(10) << "OWNED"
              << std::setw(10) << "DEFICIT"
              << std::setw(10) << "QUALITY"
              << std::setw(10) << "MIN Q"
              << "STATUS\n";
    
    auto needs = planned_needs();
    std::vector<std::string> ids;
    for (const auto& [id, _] : needs) {
        ids.push_back(id);
    }
    
    // Sort by material name
    std::sort(ids.begin(), ids.end(), [this](const std::string& a, const std::string& b) {
        const Material* ma = material_by_id(a);
        const Material* mb = material_by_id(b);
        if (!ma || !mb) return a < b;
        return ma->name < mb->name;
    });
    
    for (const auto& id : ids) {
        const Material* m = material_by_id(id);
        if (!m) continue;
        
        double required = needs.at(id);
        double owned = 0.0;
        double quality = 0.0;
        
        if (const Inventory* inv = inventory(id)) {
            owned = inv->quantity;
            quality = inv->quality;
        }
        
        double min_quality = planned_min_quality(id);
        
        std::string status = "READY";
        if (owned + 1e-9 < required) {
            status = "NEED";
        }
        if (quality + 1e-9 < min_quality) {
            if (status != "READY") {
                status += "+QUALITY";
            } else {
                status = "QUALITY";
            }
        }
        
        std::cout << std::setw(18) << m->name
                  << std::setw(10) << std::fixed << std::setprecision(2) << required
                  << std::setw(10) << std::fixed << std::setprecision(2) << owned
                  << std::setw(10) << std::fixed << std::setprecision(2) << std::max(0.0, required - owned)
                  << std::setw(10) << std::fixed << std::setprecision(0) << quality
                  << std::setw(10) << std::fixed << std::setprecision(0) << min_quality
                  << status
                  << "\n";
    }
}

// Find material by ID (non-const)
Material* App::material_by_id(const std::string& id) {
    for (auto& m : data_.materials) {
        if (m.id == id) {
            return &m;
        }
    }
    return nullptr;
}

// Find material by ID (const)
const Material* App::material_by_id(const std::string& id) const {
    for (const auto& m : data_.materials) {
        if (m.id == id) {
            return &m;
        }
    }
    return nullptr;
}

// Find blueprint by ID (non-const)
Blueprint* App::blueprint_by_id(const std::string& id) {
    for (auto& b : data_.blueprints) {
        if (b.id == id) {
            return &b;
        }
    }
    return nullptr;
}

// Find blueprint by ID (const)
const Blueprint* App::blueprint_by_id(const std::string& id) const {
    for (const auto& b : data_.blueprints) {
        if (b.id == id) {
            return &b;
        }
    }
    return nullptr;
}

// Find inventory by material ID (non-const)
Inventory* App::inventory(const std::string& material_id) {
    for (auto& inv : data_.inventory) {
        if (inv.material_id == material_id) {
            return &inv;
        }
    }
    return nullptr;
}

// Find inventory by material ID (const)
const Inventory* App::inventory(const std::string& material_id) const {
    for (const auto& inv : data_.inventory) {
        if (inv.material_id == material_id) {
            return &inv;
        }
    }
    return nullptr;
}

// Find plan item by blueprint ID
PlanItem App::plan_item(const std::string& blueprint_id) const {
    for (const auto& item : data_.plan) {
        if (item.blueprint_id == blueprint_id) {
            return item;
        }
    }
    return {};
}

// Get tracked materials
std::vector<Material> App::tracked_materials() const {
    std::vector<Material> result;
    for (const auto& m : data_.materials) {
        if (m.tracked) {
            result.push_back(m);
        }
    }
    std::sort(result.begin(), result.end(), [](const Material& a, const Material& b) {
        return a.name < b.name;
    });
    return result;
}

// Get filtered materials
std::vector<Material> App::filtered_materials() const {
    std::vector<Material> result = data_.materials;
    std::sort(result.begin(), result.end(), [](const Material& a, const Material& b) {
        return a.name < b.name;
    });
    
    if (filter_.empty()) {
        return result;
    }
    
    std::vector<Material> filtered;
    std::string lower_filter = to_lower(filter_);
    
    for (const auto& m : result) {
        if (contains_ignore_case(m.name, lower_filter) ||
            contains_ignore_case(m.kind, lower_filter) ||
            contains_ignore_case(m.tier, lower_filter)) {
            filtered.push_back(m);
        }
    }
    
    return filtered;
}

// Get filtered blueprints
std::vector<Blueprint> App::filtered_blueprints() const {
    std::vector<Blueprint> result = data_.blueprints;
    std::sort(result.begin(), result.end(), [](const Blueprint& a, const Blueprint& b) {
        return a.name < b.name;
    });
    
    if (filter_.empty()) {
        return result;
    }
    
    std::vector<Blueprint> filtered;
    std::string lower_filter = to_lower(filter_);
    
    for (const auto& b : result) {
        if (contains_ignore_case(b.name, lower_filter) ||
            contains_ignore_case(b.output, lower_filter)) {
            filtered.push_back(b);
        }
    }
    
    return filtered;
}

// Get filtered plan blueprints
std::vector<Blueprint> App::filtered_plan_blueprints() const {
    auto all = filtered_blueprints();
    std::vector<Blueprint> result;
    
    for (const auto& item : data_.plan) {
        for (const auto& b : all) {
            if (b.id == item.blueprint_id) {
                result.push_back(b);
                break;
            }
        }
    }
    
    return result;
}

// Calculate planned needs
std::map<std::string, double> App::planned_needs() const {
    std::map<std::string, double> needs;
    
    for (const auto& item : data_.plan) {
        const Blueprint* b = blueprint_by_id(item.blueprint_id);
        if (!b || item.crafts <= 0 || b->quantity <= 0) continue;
        
        double multiplier = item.crafts / b->quantity;
        for (const auto& req : b->materials) {
            needs[req.material_id] += req.quantity * multiplier;
        }
    }
    
    return needs;
}

// Calculate planned minimum quality for a material
double App::planned_min_quality(const std::string& id) const {
    double q = 0.0;
    
    for (const auto& item : data_.plan) {
        const Blueprint* b = blueprint_by_id(item.blueprint_id);
        if (!b) continue;
        
        for (const auto& req : b->materials) {
            if (req.material_id == id && req.min_quality.has_value() && req.min_quality.value() > q) {
                q = req.min_quality.value();
            }
        }
    }
    
    return q;
}

// Count tracked blueprints
int App::tracked_blueprint_count() const {
    int count = 0;
    for (const auto& b : data_.blueprints) {
        if (b.tracked) count++;
    }
    return count;
}

// Count how many tracked blueprints use a material
int App::tracked_blueprint_use_count(const std::string& material_id) const {
    int count = 0;
    for (const auto& b : data_.blueprints) {
        if (!b.tracked) continue;
        for (const auto& req : b.materials) {
            if (req.material_id == material_id) {
                count++;
                break;
            }
        }
    }
    return count;
}

// Format top locations
std::string App::format_top_locations(const std::vector<Location>& locations) const {
    return scwiki::format_top_locations(locations);
}

} // namespace scwiki
