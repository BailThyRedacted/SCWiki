package main

import "testing"

func TestMigrateLegacyInventoryCreatesExplicitMaterialTracking(t *testing.T) {
	d := Data{Version: 1, Materials: []Material{{ID: "iron", Name: "Iron"}}, Inventory: []Inventory{{MaterialID: "iron", Quantity: 12, Quality: 80}}}
	migrateData(&d)
	m := materialByIDData(d.Materials, "iron")
	if m == nil || !m.Tracked { t.Fatalf("legacy inventory should promote material to tracked") }
	if d.Version != 2 { t.Fatalf("expected data version 2, got %d", d.Version) }
	if len(m.Locations) == 0 { t.Fatalf("migration should seed location metadata") }
}

func TestTrackedBlueprintUseCountOnlyCountsTrackedBlueprints(t *testing.T) {
	a := &App{data: Data{Materials: []Material{{ID: "iron", Name: "Iron"}}, Blueprints: []Blueprint{
		{ID: "tracked", Tracked: true, Materials: []BlueprintMaterial{{MaterialID: "iron", Quantity: 1}}},
		{ID: "untracked", Tracked: false, Materials: []BlueprintMaterial{{MaterialID: "iron", Quantity: 1}}},
		{ID: "tracked2", Tracked: true, Materials: []BlueprintMaterial{{MaterialID: "iron", Quantity: 1}}},
	}}}
	if got := a.trackedBlueprintUseCount("iron"); got != 2 { t.Fatalf("expected 2 tracked blueprint uses, got %d", got) }
}

func TestTrackedMaterialsAreIndependentOfInventory(t *testing.T) {
	a := &App{data: Data{Materials: []Material{{ID: "iron", Name: "Iron", Tracked: true}, {ID: "gold", Name: "Gold"}}, Inventory: []Inventory{{MaterialID: "gold", Quantity: 10, Quality: 50}}}}
	rows := a.trackedMaterials()
	if len(rows) != 1 || rows[0].ID != "iron" { t.Fatalf("tracking should not be inferred from inventory: %#v", rows) }
}
