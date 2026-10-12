#pragma once

#include "scwiki/data.hpp"
#include "scwiki/terminal.hpp"
#include <string>
#include <vector>
#include <map>

namespace scwiki {

// Screen constants
const int SCREEN_DASHBOARD = 0;
const int SCREEN_MATERIALS = 1;
const int SCREEN_BLUEPRINTS = 2;
const int SCREEN_SCANNER = 3;
const int SCREEN_PLANNER = 4;

class App {
public:
    // Constructor
    App(const std::string& data_dir);
    
    // Main application loop
    void run();
    
    // Destructor
    ~App();
    
    // Static method to get default data directory
    static std::string default_data_dir();

private:
    Data data_;
    std::string data_dir_;
    int screen_ = SCREEN_DASHBOARD;
    int selected_ = 0;
    std::string filter_;
    std::string message_;
    bool quitting_ = false;

    // Data management
    void load();
    void save();

    // Navigation
    void open_screen(int screen);
    void handle_input(char key);
    void move(int delta);
    void move_to_end();
    int item_count() const;

    // Actions
    void toggle_material_tracking();
    void toggle_blueprint_tracking();
    void add_inventory();
    void remove_inventory();
    void prompt_filter();
    void toggle_blueprint_plan();
    void set_plan_crafts();
    void remove_plan_item();

    // Rendering
    void render();
    void render_dashboard(int height);
    void render_materials(int height);
    void render_blueprints(int height);
    void render_scanner(int height);
    void render_planner(int height);

    // Data access helpers
    Material* material_by_id(const std::string& id);
    const Material* material_by_id(const std::string& id) const;
    Blueprint* blueprint_by_id(const std::string& id);
    const Blueprint* blueprint_by_id(const std::string& id) const;
    Inventory* inventory(const std::string& material_id);
    const Inventory* inventory(const std::string& material_id) const;
    PlanItem plan_item(const std::string& blueprint_id) const;

    // Filtering helpers
    std::vector<Material> tracked_materials() const;
    std::vector<Material> filtered_materials() const;
    std::vector<Blueprint> filtered_blueprints() const;
    std::vector<Blueprint> filtered_plan_blueprints() const;

    // Planner calculations
    std::map<std::string, double> planned_needs() const;
    double planned_min_quality(const std::string& id) const;
    int tracked_blueprint_count() const;
    int tracked_blueprint_use_count(const std::string& material_id) const;

    // Utility
    std::string format_top_locations(const std::vector<Location>& locations) const;
};

} // namespace scwiki
