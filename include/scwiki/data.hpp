#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>

namespace scwiki {

// Forward declarations
struct Location;
struct Material;
struct Inventory;
struct BlueprintMaterial;
struct Blueprint;
struct PlanItem;
struct Data;

// JSON serialization helpers
void to_json(nlohmann::json& j, const Location& loc);
void from_json(const nlohmann::json& j, Location& loc);

void to_json(nlohmann::json& j, const Material& mat);
void from_json(const nlohmann::json& j, Material& mat);

void to_json(nlohmann::json& j, const Inventory& inv);
void from_json(const nlohmann::json& j, Inventory& inv);

void to_json(nlohmann::json& j, const BlueprintMaterial& bm);
void from_json(const nlohmann::json& j, BlueprintMaterial& bm);

void to_json(nlohmann::json& j, const Blueprint& bp);
void from_json(const nlohmann::json& j, Blueprint& bp);

void to_json(nlohmann::json& j, const PlanItem& item);
void from_json(const nlohmann::json& j, PlanItem& item);

void to_json(nlohmann::json& j, const Data& data);
void from_json(const nlohmann::json& j, Data& data);

// Location represents a mining location with optional spawn percentage
struct Location {
    std::string name;
    std::optional<double> spawn_percent;

    // For JSON serialization
    friend void to_json(nlohmann::json& j, const Location& loc);
    friend void from_json(const nlohmann::json& j, Location& loc);
};

// Material represents a Star Citizen mining material
struct Material {
    std::string id;
    std::string name;
    std::string tier;
    std::string kind;
    int signal_rs;
    bool tracked = false;
    std::vector<Location> locations;
    std::optional<double> quality;
    std::string notes;

    friend void to_json(nlohmann::json& j, const Material& mat);
    friend void from_json(const nlohmann::json& j, Material& mat);
};

// Inventory represents player's material stock
struct Inventory {
    std::string material_id;
    double quantity;
    double quality;

    friend void to_json(nlohmann::json& j, const Inventory& inv);
    friend void from_json(const nlohmann::json& j, Inventory& inv);
};

// BlueprintMaterial represents a material requirement for a blueprint
struct BlueprintMaterial {
    std::string material_id;
    double quantity;
    std::optional<double> min_quality;

    friend void to_json(nlohmann::json& j, const BlueprintMaterial& bm);
    friend void from_json(const nlohmann::json& j, BlueprintMaterial& bm);
};

// Blueprint represents a crafting recipe
struct Blueprint {
    std::string id;
    std::string name;
    std::string output;
    double quantity;
    bool tracked = false;
    std::vector<BlueprintMaterial> materials;
    std::string notes;

    friend void to_json(nlohmann::json& j, const Blueprint& bp);
    friend void from_json(const nlohmann::json& j, Blueprint& bp);
};

// PlanItem represents a blueprint in the crafting plan
struct PlanItem {
    std::string blueprint_id;
    double crafts;

    friend void to_json(nlohmann::json& j, const PlanItem& item);
    friend void from_json(const nlohmann::json& j, PlanItem& item);
};

// Data represents the complete application state
struct Data {
    int version = 0;
    std::string patch;
    std::vector<Material> materials;
    std::vector<Inventory> inventory;
    std::vector<Blueprint> blueprints;
    std::vector<PlanItem> plan;

    friend void to_json(nlohmann::json& j, const Data& data);
    friend void from_json(const nlohmann::json& j, Data& data);
};

// Seed materials with default data
std::vector<Material> seed_materials();

// Data migration function
void migrate_data(Data& data);

} // namespace scwiki
