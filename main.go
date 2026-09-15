package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"io"
	"math"
	"os"
	"path/filepath"
	"sort"
	"strconv"
	"strings"

	"golang.org/x/term"
)

type Location struct {
	Name  string  `json:"name"`
	Spawn float64 `json:"spawn_percent,omitempty"`
}

type Material struct {
	ID        string     `json:"id"`
	Name      string     `json:"name"`
	Tier      string     `json:"tier"`
	Kind      string     `json:"kind"`
	SignalRS  int        `json:"signal_rs"`
	Tracked   bool       `json:"tracked"`
	Locations []Location `json:"locations,omitempty"`
	Quality   *float64   `json:"quality,omitempty"`
	Notes     string     `json:"notes,omitempty"`
}

type Inventory struct {
	MaterialID string  `json:"material_id"`
	Quantity   float64 `json:"quantity"`
	Quality    float64 `json:"quality"`
}

type BlueprintMaterial struct {
	MaterialID string  `json:"material_id"`
	Quantity   float64 `json:"quantity"`
	MinQuality float64 `json:"min_quality,omitempty"`
}

type Blueprint struct {
	ID        string              `json:"id"`
	Name      string              `json:"name"`
	Output    string              `json:"output"`
	Quantity  float64             `json:"quantity"`
	Tracked   bool                `json:"tracked"`
	Materials []BlueprintMaterial `json:"materials"`
	Notes     string              `json:"notes,omitempty"`
}

type PlanItem struct {
	BlueprintID string  `json:"blueprint_id"`
	Crafts      float64 `json:"crafts"`
}

type Data struct {
	Version    int         `json:"version"`
	Patch      string      `json:"patch"`
	Materials  []Material  `json:"materials"`
	Inventory  []Inventory `json:"inventory"`
	Blueprints []Blueprint `json:"blueprints"`
	Plan       []PlanItem  `json:"plan"`
}

type App struct {
	data     Data
	dataDir  string
	screen   int
	selected int
	filter   string
	message  string
	quitting bool
}

var terminalState *term.State

const (
	screenDashboard = iota
	screenMaterials
	screenBlueprints
	screenScanner
	screenPlanner
)

func main() {
	dir, err := defaultDataDir()
	if err != nil {
		fatal(err)
	}
	app, err := load(dir)
	if err != nil {
		fatal(err)
	}
	if err := rawMode(); err != nil {
		fatal(err)
	}
	defer restoreMode()
	app.loop()
}

func defaultDataDir() (string, error) {
	if p := os.Getenv("SCWIKI_DATA"); p != "" {
		return p, os.MkdirAll(p, 0o755)
	}
	h, err := os.UserHomeDir()
	if err != nil {
		return "", err
	}
	d := filepath.Join(h, ".local", "share", "scwiki")
	return d, os.MkdirAll(d, 0o755)
}

func load(dir string) (*App, error) {
	path := filepath.Join(dir, "wiki.json")
	b, err := os.ReadFile(path)
	if os.IsNotExist(err) {
		d := Data{Version: 2, Patch: "4.10", Materials: seedMaterials()}
		if err := saveData(dir, d); err != nil {
			return nil, err
		}
		return &App{data: d, dataDir: dir}, nil
	}
	if err != nil {
		return nil, err
	}

	var d Data
	if err := json.Unmarshal(b, &d); err != nil {
		return nil, err
	}
	if d.Version == 0 {
		d.Version = 1
	}
	migrateData(&d)
	if err := saveData(dir, d); err != nil {
		return nil, err
	}
	return &App{data: d, dataDir: dir}, nil
}

func migrateData(d *Data) {
	if d.Version < 2 {
		for _, inv := range d.Inventory {
			if m := materialByIDData(d.Materials, inv.MaterialID); m != nil {
				m.Tracked = true
			}
		}
		d.Version = 2
	}

	seeds := seedMaterials()
	for i := range d.Materials {
		if len(d.Materials[i].Locations) > 0 {
			continue
		}
		if seed := materialByIDData(seeds, d.Materials[i].ID); seed != nil {
			d.Materials[i].Locations = append([]Location(nil), seed.Locations...)
		}
	}
	if d.Patch == "" {
		d.Patch = "4.10"
	}
}

func saveData(dir string, d Data) error {
	b, err := json.MarshalIndent(d, "", "  ")
	if err != nil {
		return err
	}
	path := filepath.Join(dir, "wiki.json")
	tmp := path + ".tmp"
	if err := os.WriteFile(tmp, append(b, '\n'), 0o644); err != nil {
		return err
	}
	return os.Rename(tmp, path)
}

func (a *App) save() {
	if err := saveData(a.dataDir, a.data); err != nil {
		a.message = "save failed: " + err.Error()
		return
	}
	a.message = "saved"
}

func (a *App) loop() {
	for !a.quitting {
		a.render()
		k, err := readKey(os.Stdin)
		if err != nil {
			return
		}
		a.handle(k)
	}
}

func (a *App) handle(k byte) {
	switch k {
	case 'q', 'Q':
		a.quitting = true
	case '1':
		a.openScreen(screenDashboard)
	case '2':
		a.openScreen(screenMaterials)
	case '3':
		a.openScreen(screenBlueprints)
	case '4':
		a.openScreen(screenScanner)
	case '5':
		a.openScreen(screenPlanner)
	case 'j', 'J':
		a.move(1)
	case 'k', 'K':
		a.move(-1)
	case 'g':
		a.selected = 0
	case 'G':
		a.moveToEnd()
	case 't', 'T':
		switch a.screen {
		case screenMaterials:
			a.toggleMaterialTracking()
		case screenBlueprints:
			a.toggleBlueprintTracking()
		default:
			a.message = "tracking is changed from Materials or Blueprints"
		}
	case 'a', 'A':
		switch a.screen {
		case screenMaterials:
			a.addInventory()
		case screenBlueprints:
			a.toggleBlueprintPlan()
		case screenPlanner:
			a.setPlanCrafts()
		default:
			a.message = "use Materials/Blueprints/Planner for this action"
		}
	case '/':
		a.promptFilter()
	case 's', 'S':
		a.save()
	case 'c', 'C':
		a.filter = ""
		a.selected = 0
		a.message = "filter cleared"
	case 'x', 'X':
		if a.screen == screenPlanner {
			a.removePlanItem()
		} else if a.screen == screenMaterials {
			a.removeInventory()
		}
	}
}

func (a *App) openScreen(screen int) {
	a.screen = screen
	a.selected = 0
	a.filter = ""
	a.message = ""
}

func (a *App) move(delta int) {
	n := a.itemCount()
	if n == 0 {
		return
	}
	a.selected += delta
	if a.selected < 0 {
		a.selected = 0
	}
	if a.selected >= n {
		a.selected = n - 1
	}
}

func (a *App) moveToEnd() {
	if n := a.itemCount(); n > 0 {
		a.selected = n - 1
	}
}

func (a *App) itemCount() int {
	switch a.screen {
	case screenDashboard:
		return len(a.trackedMaterials())
	case screenMaterials:
		return len(a.filteredMaterials())
	case screenBlueprints:
		return len(a.filteredBlueprints())
	case screenScanner:
		return len(a.filteredMaterials())
	case screenPlanner:
		return len(a.filteredPlanBlueprints())
	default:
		return 0
	}
}

func (a *App) toggleMaterialTracking() {
	rows := a.filteredMaterials()
	if a.selected >= len(rows) {
		return
	}
	id := rows[a.selected].ID
	m := a.materialByID(id)
	if m == nil {
		return
	}
	m.Tracked = !m.Tracked
	if m.Tracked {
		a.message = "tracked material: " + m.Name
	} else {
		a.message = "untracked material: " + m.Name
	}
	a.save()
}

func (a *App) toggleBlueprintTracking() {
	rows := a.filteredBlueprints()
	if a.selected >= len(rows) {
		return
	}
	id := rows[a.selected].ID
	b := a.blueprintByID(id)
	if b == nil {
		return
	}
	b.Tracked = !b.Tracked
	if b.Tracked {
		a.message = "tracked blueprint: " + b.Name
	} else {
		a.message = "untracked blueprint: " + b.Name
	}
	a.save()
}

func (a *App) addInventory() {
	rows := a.filteredMaterials()
	if a.selected >= len(rows) {
		return
	}
	m := rows[a.selected]
	line, err := linePrompt(fmt.Sprintf("%s quantity quality (e.g. 12.5 85): ", m.Name))
	if err != nil {
		return
	}
	fields := strings.Fields(line)
	if len(fields) < 2 {
		a.message = "expected quantity and quality"
		return
	}
	quantity, err1 := strconv.ParseFloat(fields[0], 64)
	quality, err2 := strconv.ParseFloat(fields[1], 64)
	if err1 != nil || err2 != nil || quantity < 0 || quality < 0 || quality > 1000 {
		a.message = "invalid quantity/quality"
		return
	}
	inv := a.inventory(m.ID)
	if inv == nil {
		a.data.Inventory = append(a.data.Inventory, Inventory{MaterialID: m.ID, Quantity: quantity, Quality: quality})
	} else {
		inv.Quantity = quantity
		inv.Quality = quality
	}
	a.message = "inventory updated: " + m.Name
	a.save()
}

func (a *App) removeInventory() {
	rows := a.filteredMaterials()
	if a.selected >= len(rows) {
		return
	}
	id := rows[a.selected].ID
	m := a.materialByID(id)
	a.data.Inventory = removeInventory(a.data.Inventory, id)
	if m != nil {
		a.message = "inventory cleared: " + m.Name
	}
	a.save()
}

func (a *App) promptFilter() {
	value, err := linePrompt("filter: ")
	if err != nil {
		return
	}
	a.filter = strings.TrimSpace(value)
	a.selected = 0
	a.message = "filter: " + a.filter
}

func (a *App) toggleBlueprintPlan() {
	rows := a.filteredBlueprints()
	if a.selected >= len(rows) {
		return
	}
	id := rows[a.selected].ID
	for i := range a.data.Plan {
		if a.data.Plan[i].BlueprintID == id {
			a.data.Plan = append(a.data.Plan[:i], a.data.Plan[i+1:]...)
			a.message = "removed from plan: " + rows[a.selected].Name
			a.save()
			return
		}
	}
	a.data.Plan = append(a.data.Plan, PlanItem{BlueprintID: id, Crafts: rows[a.selected].Quantity})
	a.message = "added to plan: " + rows[a.selected].Name
	a.save()
}

func (a *App) setPlanCrafts() {
	rows := a.filteredPlanBlueprints()
	if a.selected >= len(rows) {
		return
	}
	id := rows[a.selected].ID
	line, err := linePrompt("output units (0 removes): ")
	if err != nil {
		return
	}
	value, err := strconv.ParseFloat(strings.TrimSpace(line), 64)
	if err != nil || value < 0 {
		a.message = "invalid craft quantity"
		return
	}
	for i := range a.data.Plan {
		if a.data.Plan[i].BlueprintID == id {
			if value == 0 {
				a.data.Plan = append(a.data.Plan[:i], a.data.Plan[i+1:]...)
			} else {
				a.data.Plan[i].Crafts = value
			}
			a.save()
			return
		}
	}
	if value > 0 {
		a.data.Plan = append(a.data.Plan, PlanItem{BlueprintID: id, Crafts: value})
		a.save()
	}
}

func (a *App) removePlanItem() {
	rows := a.filteredPlanBlueprints()
	if a.selected >= len(rows) {
		return
	}
	id := rows[a.selected].ID
	for i := range a.data.Plan {
		if a.data.Plan[i].BlueprintID == id {
			a.data.Plan = append(a.data.Plan[:i], a.data.Plan[i+1:]...)
			a.message = "removed from plan: " + rows[a.selected].Name
			a.save()
			return
		}
	}
}

func (a *App) render() {
	clearScreen()
	width, height := termSize()
	fmt.Printf("SCWIKI • local Star Citizen wiki • patch %s\n", a.data.Patch)
	fmt.Println("1 Dashboard  2 Materials  3 Blueprints  4 RS Tracker  5 Planner  |  j/k move  g/G top/end")
	fmt.Println("t track/untrack  a add/set  x remove  / filter  c clear  s save  q quit")
	fmt.Println(strings.Repeat("─", min(width, 120)))

	switch a.screen {
	case screenDashboard:
		a.renderDashboard(height)
	case screenMaterials:
		a.renderMaterials(height)
	case screenBlueprints:
		a.renderBlueprints(height)
	case screenScanner:
		a.renderScanner(height)
	case screenPlanner:
		a.renderPlanner(height)
	}

	fmt.Println(strings.Repeat("─", min(width, 120)))
	if a.message != "" {
		fmt.Println(a.message)
	} else {
		fmt.Println("Tip: use t to track materials/blueprints; RS Tracker uses the selected material — no signal search required.")
	}
}

func (a *App) renderDashboard(height int) {
	rows := a.trackedMaterials()
	fmt.Printf("DASHBOARD • %d tracked materials • %d tracked blueprints\n\n", len(rows), a.trackedBlueprintCount())
	if len(rows) == 0 {
		fmt.Println("No materials are tracked yet.")
		fmt.Println("Go to Materials, select a material, and press t.")
		return
	}

	fmt.Printf("%-18s %8s %-46s %12s\n", "MATERIAL", "RS", "TOP 3 LOCATIONS", "TRACKED BPs")
	fmt.Println(strings.Repeat("─", 92))
	start, end := window(a.selected, len(rows), height-8)
	for i := start; i < end; i++ {
		m := rows[i]
		locations := formatTopLocations(m.Locations)
		fmt.Printf("%s%-18s %8d %-46s %12d\n", selmark(i, a.selected), m.Name, m.SignalRS, locations, a.trackedBlueprintUseCount(m.ID))
	}
}

func (a *App) renderMaterials(height int) {
	rows := a.filteredMaterials()
	fmt.Printf("MATERIALS • %d shown\n\n%-18s %-5s %-20s %8s %10s %10s\n", len(rows), "NAME", "TIER", "KIND", "RS", "TRACKED", "OWNED")
	start, end := window(a.selected, len(rows), height-7)
	for i := start; i < end; i++ {
		m := rows[i]
		tracked := "no"
		if m.Tracked {
			tracked = "yes"
		}
		owned := "-"
		if inv := a.inventory(m.ID); inv != nil {
			owned = fmt.Sprintf("%.2f", inv.Quantity)
		}
		fmt.Printf("%s%-18s %-5s %-20s %8d %10s %10s\n", selmark(i, a.selected), m.Name, m.Tier, m.Kind, m.SignalRS, tracked, owned)
	}
	fmt.Println("\n`t` tracking is independent from inventory; `a` updates owned quantity/quality; `x` clears inventory.")
}

func (a *App) renderBlueprints(height int) {
	rows := a.filteredBlueprints()
	fmt.Printf("BLUEPRINTS • %d shown\n\n%-28s %-18s %9s %10s\n", len(rows), "NAME", "OUTPUT", "TRACKED", "IN PLAN")
	start, end := window(a.selected, len(rows), height-15)
	for i := start; i < end; i++ {
		b := rows[i]
		tracked := "no"
		if b.Tracked {
			tracked = "yes"
		}
		inPlan := "no"
		if a.planItem(b.ID).BlueprintID != "" {
			inPlan = "yes"
		}
		fmt.Printf("%s%-28s %-18s %9s %10s\n", selmark(i, a.selected), b.Name, b.Output, tracked, inPlan)
	}
	if len(rows) == 0 {
		return
	}
	b := rows[a.selected]
	fmt.Printf("\nSELECTED • %s → %s x%.2f\n%-18s %10s %10s %10s\n", b.Name, b.Output, b.Quantity, "MATERIAL", "REQUIRED", "OWNED", "MIN Q")
	for _, req := range b.Materials {
		m := a.materialByID(req.MaterialID)
		if m == nil {
			continue
		}
		owned, quality := 0.0, 0.0
		if inv := a.inventory(req.MaterialID); inv != nil {
			owned, quality = inv.Quantity, inv.Quality
		}
		minQuality := "-"
		if req.MinQuality > 0 {
			minQuality = fmt.Sprintf("%.0f", req.MinQuality)
		}
		status := ""
		if owned+1e-9 < req.Quantity {
			status = " *NEED*"
		}
		fmt.Printf("%-18s %10.2f %10.2f %10.0f %10s%s\n", m.Name, req.Quantity, owned, quality, minQuality, status)
	}
	fmt.Println("\n`t` track this blueprint; `a` add/remove it from the crafting plan.")
}

func (a *App) renderScanner(height int) {
	rows := a.filteredMaterials()
	fmt.Printf("RS TRACKER • select a material, then read its signature\n\n%-3s %-20s %-8s %-5s %-46s\n", "", "MATERIAL", "BASE RS", "TIER", "QUICK INFO")
	fmt.Println(strings.Repeat("─", 88))
	start, end := window(a.selected, len(rows), height-9)
	for i := start; i < end; i++ {
		m := rows[i]
		locations := formatTopLocations(m.Locations)
		fmt.Printf("%s%-20s %-8d %-5s %-46s\n", selmark(i, a.selected), m.Name, m.SignalRS, m.Tier, "Top: "+locations)
	}
	if len(rows) > 0 {
		m := rows[a.selected]
		fmt.Printf("\nSELECTED MATERIAL: %s\nRS SIGNATURE: %d\n\nUse j/k to select the material. `/` still filters material names; there is no RS-value search.\n", m.Name, m.SignalRS)
	}
}

func (a *App) renderPlanner(height int) {
	rows := a.filteredPlanBlueprints()
	fmt.Printf("CRAFTING PLANNER • %d planned blueprints\n\n%-28s %12s %10s\n", len(rows), "BLUEPRINT", "OUTPUT UNITS", "BATCH")
	start, end := window(a.selected, len(rows), height-15)
	for i := start; i < end; i++ {
		b := rows[i]
		plan := a.planItem(b.ID)
		fmt.Printf("%s%-28s %12.2f %10.2f\n", selmark(i, a.selected), b.Name, plan.Crafts, b.Quantity)
	}

	fmt.Println("\nMATERIAL ROLL-UP")
	fmt.Printf("%-18s %10s %10s %10s %10s %s\n", "MATERIAL", "REQUIRED", "OWNED", "DEFICIT", "QUALITY", "STATUS")
	needs := a.plannedNeeds()
	ids := make([]string, 0, len(needs))
	for id := range needs {
		ids = append(ids, id)
	}
	sort.Slice(ids, func(i, j int) bool {
		mi := a.materialByID(ids[i])
		mj := a.materialByID(ids[j])
		if mi == nil || mj == nil {
			return ids[i] < ids[j]
		}
		return mi.Name < mj.Name
	})
	for _, id := range ids {
		m := a.materialByID(id)
		if m == nil {
			continue
		}
		required := needs[id]
		owned, quality := 0.0, 0.0
		if inv := a.inventory(id); inv != nil {
			owned, quality = inv.Quantity, inv.Quality
		}
		minQuality := a.plannedMinQuality(id)
		status := "READY"
		if owned+1e-9 < required {
			status = "NEED"
		}
		if quality+1e-9 < minQuality {
			if status != "READY" {
				status += "+QUALITY"
			} else {
				status = "QUALITY"
			}
		}
		fmt.Printf("%-18s %10.2f %10.2f %10.2f %10.0f %s\n", m.Name, required, owned, math.Max(0, required-owned), quality, status)
	}
}

func (a *App) trackedMaterials() []Material {
	rows := make([]Material, 0)
	for _, m := range a.data.Materials {
		if m.Tracked {
			rows = append(rows, m)
		}
	}
	sort.Slice(rows, func(i, j int) bool { return rows[i].Name < rows[j].Name })
	return rows
}

func (a *App) filteredMaterials() []Material {
	rows := append([]Material(nil), a.data.Materials...)
	sort.Slice(rows, func(i, j int) bool { return rows[i].Name < rows[j].Name })
	if a.filter == "" {
		return rows
	}
	f := strings.ToLower(a.filter)
	out := rows[:0]
	for _, m := range rows {
		if strings.Contains(strings.ToLower(m.Name), f) || strings.Contains(strings.ToLower(m.Kind), f) || strings.Contains(strings.ToLower(m.Tier), f) {
			out = append(out, m)
		}
	}
	return out
}

func (a *App) filteredBlueprints() []Blueprint {
	rows := append([]Blueprint(nil), a.data.Blueprints...)
	sort.Slice(rows, func(i, j int) bool { return rows[i].Name < rows[j].Name })
	if a.filter == "" {
		return rows
	}
	f := strings.ToLower(a.filter)
	out := rows[:0]
	for _, b := range rows {
		if strings.Contains(strings.ToLower(b.Name), f) || strings.Contains(strings.ToLower(b.Output), f) {
			out = append(out, b)
		}
	}
	return out
}

func (a *App) filteredPlanBlueprints() []Blueprint {
	all := a.filteredBlueprints()
	out := make([]Blueprint, 0, len(a.data.Plan))
	for _, item := range a.data.Plan {
		for _, b := range all {
			if b.ID == item.BlueprintID {
				out = append(out, b)
				break
			}
		}
	}
	return out
}

func (a *App) plannedNeeds() map[string]float64 {
	needs := map[string]float64{}
	for _, item := range a.data.Plan {
		b := a.blueprintByID(item.BlueprintID)
		if b == nil || item.Crafts <= 0 || b.Quantity <= 0 {
			continue
		}
		multiplier := item.Crafts / b.Quantity
		for _, req := range b.Materials {
			needs[req.MaterialID] += req.Quantity * multiplier
		}
	}
	return needs
}

func (a *App) plannedMinQuality(id string) float64 {
	q := 0.0
	for _, item := range a.data.Plan {
		b := a.blueprintByID(item.BlueprintID)
		if b == nil {
			continue
		}
		for _, req := range b.Materials {
			if req.MaterialID == id && req.MinQuality > q {
				q = req.MinQuality
			}
		}
	}
	return q
}

func (a *App) trackedBlueprintCount() int {
	count := 0
	for _, b := range a.data.Blueprints {
		if b.Tracked {
			count++
		}
	}
	return count
}

func (a *App) trackedBlueprintUseCount(materialID string) int {
	count := 0
	for _, b := range a.data.Blueprints {
		if !b.Tracked {
			continue
		}
		for _, req := range b.Materials {
			if req.MaterialID == materialID {
				count++
				break
			}
		}
	}
	return count
}

func (a *App) inventory(id string) *Inventory {
	for i := range a.data.Inventory {
		if a.data.Inventory[i].MaterialID == id {
			return &a.data.Inventory[i]
		}
	}
	return nil
}

func removeInventory(rows []Inventory, id string) []Inventory {
	out := rows[:0]
	for _, row := range rows {
		if row.MaterialID != id {
			out = append(out, row)
		}
	}
	return out
}

func (a *App) materialByID(id string) *Material {
	for i := range a.data.Materials {
		if a.data.Materials[i].ID == id {
			return &a.data.Materials[i]
		}
	}
	return nil
}

func (a *App) blueprintByID(id string) *Blueprint {
	for i := range a.data.Blueprints {
		if a.data.Blueprints[i].ID == id {
			return &a.data.Blueprints[i]
		}
	}
	return nil
}

func (a *App) planItem(id string) PlanItem {
	for _, item := range a.data.Plan {
		if item.BlueprintID == id {
			return item
		}
	}
	return PlanItem{}
}

func formatTopLocations(locations []Location) string {
	if len(locations) == 0 {
		return "No location data"
	}
	rows := append([]Location(nil), locations...)
	sort.SliceStable(rows, func(i, j int) bool {
		if rows[i].Spawn == rows[j].Spawn {
			return rows[i].Name < rows[j].Name
		}
		return rows[i].Spawn > rows[j].Spawn
	})
	if len(rows) > 3 {
		rows = rows[:3]
	}
	parts := make([]string, 0, len(rows))
	for _, loc := range rows {
		if loc.Spawn > 0 {
			parts = append(parts, fmt.Sprintf("%s %.0f%%", loc.Name, loc.Spawn))
		} else {
			parts = append(parts, loc.Name)
		}
	}
	return strings.Join(parts, " • ")
}

func materialByIDData(rows []Material, id string) *Material {
	for i := range rows {
		if rows[i].ID == id {
			return &rows[i]
		}
	}
	return nil
}

func seedMaterials() []Material {
	type seed struct {
		id, name, tier string
		rs             int
		locations      []Location
	}
	mk := func(names ...string) []Location {
		out := make([]Location, 0, len(names))
		for _, name := range names {
			out = append(out, Location{Name: name})
		}
		return out
	}
	rows := []seed{
		{"quantainium", "Quantainium", "S", 3170, mk("Lyria", "Yela Belt", "Aaron Halo")},
		{"stileron", "Stileron", "A", 3185, mk("Pyro Belt", "Bloom", "Vuur")},
		{"savrilium", "Savrilium", "A", 3200, mk("Keeger Belt", "Pyro Belt", "Delamar")},
		{"ouratite", "Ouratite", "A", 3370, mk("Aberdeen", "Magda", "Arial")},
		{"riccite", "Riccite", "B", 3385, mk("Pyro Belt", "Vuur", "Bloom")},
		{"lindinium", "Lindinium", "B", 3400, mk("Yela", "Lyria", "Daymar")},
		{"beryl", "Beryl", "A", 3540, mk("Aberdeen", "Lyria", "Yela")},
		{"taranite", "Taranite", "A", 3555, mk("Aberdeen", "Lyria", "Aaron Halo")},
		{"borase", "Borase", "B", 3570, mk("Keeger Belt", "Glaciem Ring", "Pyro Belt")},
		{"gold", "Gold", "A", 3585, mk("Glaciem Ring", "Keeger Belt", "Aaron Halo")},
		{"bexalite", "Bexalite", "S", 3600, mk("Glaciem Ring", "Keeger Belt", "Adir")},
		{"laranite", "Laranite", "A", 3825, mk("Lyria", "Yela", "Aaron Halo")},
		{"aslarite", "Aslarite", "B", 3840, mk("Aberdeen", "Lyria", "Yela")},
		{"titanium", "Titanium", "B", 3855, mk("Aberdeen", "Lyria", "Yela")},
		{"tungsten", "Tungsten", "B", 3870, mk("Lyria", "Yela", "Cellin")},
		{"agricium", "Agricium", "A", 3885, mk("Aberdeen", "Lyria", "Magda")},
		{"torite", "Torite", "B", 3900, mk("Pyro Belt", "Bloom", "Vuur")},
		{"hephaestanite", "Hephaestanite", "B", 4180, mk("Aberdeen", "Lyria", "Yela")},
		{"tin", "Tin", "C", 4195, mk("Aberdeen", "Yela", "Lyria")},
		{"quartz", "Quartz", "C", 4210, mk("Aberdeen", "Yela", "Lyria")},
		{"corundum", "Corundum", "C", 4225, mk("Aberdeen", "Arial", "Yela")},
		{"copper", "Copper", "C", 4240, mk("Aberdeen", "Lyria", "Yela")},
		{"silicon", "Silicon", "C", 4255, mk("Aberdeen", "Lyria", "Yela")},
		{"iron", "Iron", "C", 4270, mk("Aberdeen", "Lyria", "Yela")},
		{"aluminium", "Aluminium", "C", 4285, mk("Aberdeen", "Arial", "Magda")},
		{"ice", "Ice", "C", 4300, mk("Euterpe", "Calliope", "Clio")},
	}
	out := make([]Material, 0, len(rows))
	for _, row := range rows {
		out = append(out, Material{ID: row.id, Name: row.name, Tier: row.tier, Kind: "Ship Mineable Ore", SignalRS: row.rs, Locations: row.locations})
	}
	return out
}

func linePrompt(prompt string) (string, error) {
	restoreMode()
	defer func() { _ = rawMode() }()
	fmt.Print("\r" + prompt)
	return strings.TrimSpace(mustReadLine()), nil
}

func mustReadLine() string {
	reader := bufio.NewReader(os.Stdin)
	value, _ := reader.ReadString('\n')
	return value
}

func rawMode() error {
	state, err := term.MakeRaw(int(os.Stdin.Fd()))
	if err != nil {
		return err
	}
	terminalState = state
	return nil
}

func restoreMode() {
	if terminalState != nil {
		_ = term.Restore(int(os.Stdin.Fd()), terminalState)
		terminalState = nil
	}
}

func readKey(reader io.Reader) (byte, error) {
	b := make([]byte, 1)
	_, err := reader.Read(b)
	if err != nil {
		return 0, err
	}
	return b[0], nil
}

func clearScreen() {
	fmt.Print("\x1b[2J\x1b[H")
}

func termSize() (int, int) {
	return 100, 30
}

func window(selected, count, rows int) (int, int) {
	if rows < 5 {
		rows = 5
	}
	if count <= rows {
		return 0, count
	}
	start := selected - rows/2
	if start < 0 {
		start = 0
	}
	if start+rows > count {
		start = count - rows
	}
	return start, start + rows
}

func selmark(index, selected int) string {
	if index == selected {
		return ">"
	}
	return " "
}

func min(a, b int) int {
	if a < b {
		return a
	}
	return b
}

func fatal(err error) {
	fmt.Fprintln(os.Stderr, err)
	os.Exit(1)
}
