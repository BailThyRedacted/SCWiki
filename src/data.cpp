#include "scwiki/data.hpp"
#include <nlohmann/json.hpp>
#include <vector>
#include <string>
#include <algorithm>

namespace scwiki {

// Location JSON serialization
void to_json(nlohmann::json& j, const Location& loc) {
    j = nlohmann::json{
        {"name", loc.name}
    };
    if (loc.spawn_percent.has_value()) {
        j["spawn_percent"] = loc.spawn_percent.value();
    }
}

void from_json(const nlohmann::json& j, Location& loc) {
    j.at("name").get_to(loc.name);
    if (j.contains("spawn_percent")) {
        loc.spawn_percent = j.at("spawn_percent").get<double>();
    } else {
        loc.spawn_percent = std::nullopt;
    }
}

// Material JSON serialization
void to_json(nlohmann::json& j, const Material& mat) {
    j = nlohmann::json{
        {"id", mat.id},
        {"name", mat.name},
        {"tier", mat.tier},
        {"kind", mat.kind},
        {"signal_rs", mat.signal_rs},
        {"tracked", mat.tracked},
        {"locations", mat.locations}
    };
    if (mat.quality.has_value()) {
        j["quality"] = mat.quality.value();
    }
    if (!mat.notes.empty()) {
        j["notes"] = mat.notes;
    }
}

void from_json(const nlohmann::json& j, Material& mat) {
    j.at("id").get_to(mat.id);
    j.at("name").get_to(mat.name);
    j.at("tier").get_to(mat.tier);
    j.at("kind").get_to(mat.kind);
    j.at("signal_rs").get_to(mat.signal_rs);
    
    // Handle optional fields
    if (j.contains("tracked")) {
        j.at("tracked").get_to(mat.tracked);
    } else {
        mat.tracked = false;
    }
    
    if (j.contains("locations")) {
        j.at("locations").get_to(mat.locations);
    }
    
    if (j.contains("quality")) {
        mat.quality = j.at("quality").get<double>();
    } else {
        mat.quality = std::nullopt;
    }
    
    if (j.contains("notes")) {
        j.at("notes").get_to(mat.notes);
    } else {
        mat.notes.clear();
    }
}

// Inventory JSON serialization
void to_json(nlohmann::json& j, const Inventory& inv) {
    j = nlohmann::json{
        {"material_id", inv.material_id},
        {"quantity", inv.quantity},
        {"quality", inv.quality}
    };
}

void from_json(const nlohmann::json& j, Inventory& inv) {
    j.at("material_id").get_to(inv.material_id);
    j.at("quantity").get_to(inv.quantity);
    j.at("quality").get_to(inv.quality);
}

// BlueprintMaterial JSON serialization
void to_json(nlohmann::json& j, const BlueprintMaterial& bm) {
    j = nlohmann::json{
        {"material_id", bm.material_id},
        {"quantity", bm.quantity}
    };
    if (bm.min_quality.has_value()) {
        j["min_quality"] = bm.min_quality.value();
    }
}

void from_json(const nlohmann::json& j, BlueprintMaterial& bm) {
    j.at("material_id").get_to(bm.material_id);
    j.at("quantity").get_to(bm.quantity);
    if (j.contains("min_quality")) {
        bm.min_quality = j.at("min_quality").get<double>();
    } else {
        bm.min_quality = std::nullopt;
    }
}

// Blueprint JSON serialization
void to_json(nlohmann::json& j, const Blueprint& bp) {
    j = nlohmann::json{
        {"id", bp.id},
        {"name", bp.name},
        {"output", bp.output},
        {"quantity", bp.quantity},
        {"tracked", bp.tracked},
        {"materials", bp.materials}
    };
    if (!bp.notes.empty()) {
        j["notes"] = bp.notes;
    }
}

void from_json(const nlohmann::json& j, Blueprint& bp) {
    j.at("id").get_to(bp.id);
    j.at("name").get_to(bp.name);
    j.at("output").get_to(bp.output);
    j.at("quantity").get_to(bp.quantity);
    
    if (j.contains("tracked")) {
        j.at("tracked").get_to(bp.tracked);
    } else {
        bp.tracked = false;
    }
    
    if (j.contains("materials")) {
        j.at("materials").get_to(bp.materials);
    }
    
    if (j.contains("notes")) {
        j.at("notes").get_to(bp.notes);
    } else {
        bp.notes.clear();
    }
}

// PlanItem JSON serialization
void to_json(nlohmann::json& j, const PlanItem& item) {
    j = nlohmann::json{
        {"blueprint_id", item.blueprint_id},
        {"crafts", item.crafts}
    };
}

void from_json(const nlohmann::json& j, PlanItem& item) {
    j.at("blueprint_id").get_to(item.blueprint_id);
    j.at("crafts").get_to(item.crafts);
}

// Data JSON serialization
void to_json(nlohmann::json& j, const Data& data) {
    j = nlohmann::json{
        {"version", data.version},
        {"patch", data.patch},
        {"materials", data.materials},
        {"inventory", data.inventory},
        {"blueprints", data.blueprints},
        {"plan", data.plan}
    };
}

void from_json(const nlohmann::json& j, Data& data) {
    if (j.contains("version")) {
        j.at("version").get_to(data.version);
    } else {
        data.version = 0;
    }
    
    if (j.contains("patch")) {
        j.at("patch").get_to(data.patch);
    } else {
        data.patch.clear();
    }
    
    if (j.contains("materials")) {
        j.at("materials").get_to(data.materials);
    }
    
    if (j.contains("inventory")) {
        j.at("inventory").get_to(data.inventory);
    }
    
    if (j.contains("blueprints")) {
        j.at("blueprints").get_to(data.blueprints);
    }
    
    if (j.contains("plan")) {
        j.at("plan").get_to(data.plan);
    }
}

// Helper struct for seed material data
struct SeedMaterialDef {
    std::string id;
    std::string name;
    std::string tier;
    int rs;
    std::vector<std::string> location_names;
};

// Seed materials with default data - matches Go version exactly
std::vector<Material> seed_materials() {
    std::vector<SeedMaterialDef> seeds = {
        {"quantainium", "Quantainium", "S", 3170, {"Lyria", "Yela Belt", "Aaron Halo"}},
        {"stileron", "Stileron", "A", 3185, {"Pyro Belt", "Bloom", "Vuur"}},
        {"savrilium", "Savrilium", "A", 3200, {"Keeger Belt", "Pyro Belt", "Delamar"}},
        {"ouratite", "Ouratite", "A", 3370, {"Aberdeen", "Magda", "Arial"}},
        {"riccite", "Riccite", "B", 3385, {"Pyro Belt", "Vuur", "Bloom"}},
        {"lindinium", "Lindinium", "B", 3400, {"Yela", "Lyria", "Daymar"}},
        {"beryl", "Beryl", "A", 3540, {"Aberdeen", "Lyria", "Yela"}},
        {"taranite", "Taranite", "A", 3555, {"Aberdeen", "Lyria", "Aaron Halo"}},
        {"borase", "Borase", "B", 3570, {"Keeger Belt", "Glaciem Ring", "Pyro Belt"}},
        {"gold", "Gold", "A", 3585, {"Glaciem Ring", "Keeger Belt", "Aaron Halo"}},
        {"bexalite", "Bexalite", "S", 3600, {"Glaciem Ring", "Keeger Belt", "Adir"}},
        {"laranite", "Laranite", "A", 3825, {"Lyria", "Yela", "Aaron Halo"}},
        {"aslarite", "Aslarite", "B", 3840, {"Aberdeen", "Lyria", "Yela"}},
        {"titanium", "Titanium", "B", 3855, {"Aberdeen", "Lyria", "Yela"}},
        {"tungsten", "Tungsten", "B", 3870, {"Lyria", "Yela", "Cellin"}},
        {"agricium", "Agricium", "A", 3885, {"Aberdeen", "Lyria", "Magda"}},
        {"torite", "Torite", "B", 3900, {"Pyro Belt", "Bloom", "Vuur"}},
        {"hephaestanite", "Hephaestanite", "B", 4180, {"Aberdeen", "Lyria", "Yela"}},
        {"tin", "Tin", "C", 4195, {"Aberdeen", "Yela", "Lyria"}},
        {"quartz", "Quartz", "C", 4210, {"Aberdeen", "Yela", "Lyria"}},
        {"corundum", "Corundum", "C", 4225, {"Aberdeen", "Arial", "Yela"}},
        {"copper", "Copper", "C", 4240, {"Aberdeen", "Lyria", "Yela"}},
        {"silicon", "Silicon", "C", 4255, {"Aberdeen", "Lyria", "Yela"}},
        {"iron", "Iron", "C", 4270, {"Aberdeen", "Lyria", "Yela"}},
        {"aluminium", "Aluminium", "C", 4285, {"Aberdeen", "Arial", "Magda"}},
        {"ice", "Ice", "C", 4300, {"Euterpe", "Calliope", "Clio"}}
    };

    std::vector<Material> result;
    result.reserve(seeds.size());

    for (const auto& seed : seeds) {
        Material mat;
        mat.id = seed.id;
        mat.name = seed.name;
        mat.tier = seed.tier;
        mat.kind = "Ship Mineable Ore";
        mat.signal_rs = seed.rs;
        mat.tracked = false;
        mat.quality = std::nullopt;
        mat.notes.clear();

        for (const auto& loc_name : seed.location_names) {
            Location loc;
            loc.name = loc_name;
            loc.spawn_percent = std::nullopt;
            mat.locations.push_back(loc);
        }

        result.push_back(mat);
    }

    return result;
}

// Data migration: version 1 -> 2
void migrate_data(Data& data) {
    // Migration from version 1 to 2
    if (data.version < 2) {
        // Legacy inventory should promote materials to tracked
        for (const auto& inv : data.inventory) {
            for (auto& mat : data.materials) {
                if (mat.id == inv.material_id) {
                    mat.tracked = true;
                    break;
                }
            }
        }

        // Seed location metadata for materials that don't have it
        auto seeds = seed_materials();
        for (auto& mat : data.materials) {
            if (mat.locations.empty()) {
                for (const auto& seed : seeds) {
                    if (seed.id == mat.id) {
                        mat.locations = seed.locations;
                        break;
                    }
                }
            }
        }

        data.version = 2;
    }

    // Ensure patch is set
    if (data.patch.empty()) {
        data.patch = "4.10";
    }
}

} // namespace scwiki
