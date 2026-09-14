# SCWiki User Guide

SCWiki is a small, local terminal user interface (TUI) for tracking Star Citizen mining materials, blueprints, inventory, scanner signatures, and crafting requirements.

The application does not need an internet connection while it is running. Your personal data is kept in a local JSON file.

## Requirements

You need:

- Git, if you are cloning the source repository.
- Go 1.20 or newer to build SCWiki from source.
- A terminal that supports standard ANSI escape sequences.

Linux and macOS are the primary targets. Windows users should use a terminal such as Windows Terminal, PowerShell, or another ANSI-capable terminal.

## 1. Get the program

### Option A: Clone the GitHub repository

```bash
git clone https://github.com/BailThyRedacted/SCWiki.git
cd SCWiki
```

### Option B: Download a copy of the repository

From the GitHub repository, use **Code -> Download ZIP**, extract it, and open a terminal in the extracted directory.

## 2. Build SCWiki

From the repository directory, run:

```bash
go build -ldflags='-s -w' -o scwiki .
```

This produces a small executable named `scwiki` in the current directory.

On Windows, Go will normally produce `scwiki.exe`:

```powershell
go build -ldflags="-s -w" -o scwiki.exe .
```

You only need to build the program again when you update the source code. Your saved data is separate from the executable.

## 3. Run SCWiki

### Linux or macOS

From the repository directory:

```bash
./scwiki
```

You can also copy the executable somewhere on your `PATH`, for example:

```bash
sudo install -m 755 scwiki /usr/local/bin/scwiki
```

Then launch it from any directory with:

```bash
scwiki
```

### Windows

From PowerShell in the repository directory:

```powershell
.\scwiki.exe
```

## 4. First launch

SCWiki opens directly in the terminal. It loads the bundled wiki data and your local saved state.

The main views are:

| Key | View | Purpose |
|---|---|---|
| `1` | Materials | Browse mining materials, scanner signatures, and tracked quantities/quality. |
| `2` | Blueprints | Browse available blueprints and their material requirements. |
| `3` | Scanner | Enter a scanner RS value and estimate the matching material and node count. |
| `4` | Tracked | Review your tracked material inventory. |
| `5` | Planner | Build a crafting plan and see the resulting material requirements and deficits. |

Use `q` to quit.

## 5. Basic navigation

The TUI is designed for keyboard-only use.

- `j` - move down
- `k` - move up
- `/` - filter/search the current list
- `s` - save your current state
- `q` - quit

The exact prompt shown at the bottom of the screen indicates when SCWiki is waiting for text or numeric input.

## 6. Tracking materials

Use the Materials or Tracked view to maintain the inventory you actually have available for crafting.

For each tracked material, the important values are:

- **Quantity** - the amount you currently have.
- **Quality** - the material quality on the game's 0-1000 scale.

Keeping these values accurate is what makes the crafting planner useful.

Press `s` after making changes to explicitly save the current state.

## 7. Using the scanner lookup

Open the Scanner view with:

```text
3
```

Press `r` to enter an RS value.

SCWiki uses the configured material signatures to estimate:

- the most likely material
- the approximate number of nodes/rocks producing that scanner reading

The lookup is intended as a quick local reference rather than a replacement for an in-game scan.

## 8. Creating a crafting plan

Open the Crafting Planner with:

```text
5
```

Move to a blueprint using `j` and `k`.

Press `t` to add or remove the selected blueprint from the plan.

Press `a` to specify how many **output units** you want to produce.

For example, if a blueprint produces 5 items per craft and you enter 10 output units, SCWiki calculates the recipe for two crafts rather than treating 10 as ten recipe executions.

Press `x` to remove the selected blueprint from the plan.

Press `0` when prompted to clear a planned quantity.

## 9. Reading the planner

The planner rolls all selected blueprints into one material requirement list.

For each material it can show:

- **Required** - total material needed for the current plan.
- **Owned** - quantity currently tracked in your inventory.
- **Deficit** - additional quantity you need to obtain.
- **Quality** - quality of your tracked material.
- **Minimum quality** - quality threshold required by the planned recipes.
- **Status** - whether the current inventory is sufficient.

Typical statuses are:

- `READY` - enough quantity and acceptable quality.
- `NEED` - not enough quantity.
- `QUALITY` - enough quantity, but the tracked material quality is below the requirement.

Quantity and quality are evaluated separately. Having enough kilograms/units does not make a low-quality material count as sufficient.

## 10. Understanding quality requirements

When multiple planned blueprints use the same material, SCWiki evaluates the material against the highest minimum-quality requirement among those recipes.

For example:

```text
Blueprint A requires: Material X, minimum quality 300
Blueprint B requires: Material X, minimum quality 700
```

A tracked Material X quality of 500 is sufficient for Blueprint A but not Blueprint B, so the rolled-up planner can mark Material X as `QUALITY`.

## 11. Where your data is stored

By default, SCWiki stores its local data at:

```text
~/.local/share/scwiki/wiki.json
```

This file contains your local tracking/planning state and the wiki data loaded by the application.

You can choose another data file by setting `SCWIKI_DATA` before starting the program.

Linux/macOS:

```bash
SCWIKI_DATA=/path/to/wiki.json ./scwiki
```

Windows PowerShell:

```powershell
$env:SCWIKI_DATA = "C:\path\to\wiki.json"
.\scwiki.exe
```

This is useful when you want separate profiles, a portable data file, or a backup location.

## 12. Backing up your data

Because the data is stored in a normal JSON file, backing it up is straightforward.

Linux/macOS example:

```bash
cp ~/.local/share/scwiki/wiki.json ~/scwiki-backup.json
```

You can restore it by copying the backup back to the SCWiki data location, or by using `SCWIKI_DATA` to point SCWiki at the backup file.

## 13. Updating SCWiki

If you installed from Git:

```bash
git pull
```

Rebuild the executable:

```bash
go build -ldflags='-s -w' -o scwiki .
```

Your local data file is outside the Git working tree by default, so updating the source does not normally overwrite your personal tracking state.

## 14. Troubleshooting

### `go: command not found`

Install Go and verify it is available on your `PATH`:

```bash
go version
```

### `permission denied` when running `./scwiki`

Make the executable executable:

```bash
chmod +x scwiki
```

Then run:

```bash
./scwiki
```

### The terminal display looks broken

Use a modern terminal emulator with ANSI escape support. Resize the terminal to a reasonable width and restart SCWiki after resizing if the layout does not immediately recover.

### My tracked data is missing

Check that SCWIKI_DATA is not pointing at a different file:

```bash
echo "$SCWIKI_DATA"
```

Then check the default file:

```bash
ls -l ~/.local/share/scwiki/wiki.json
```

On Windows PowerShell:

```powershell
echo $env:SCWIKI_DATA
```

### I changed data but it did not persist

Press `s` before quitting. SCWiki is deliberately local-first and does not continuously write to disk on every keystroke.

## 15. Recommended workflow

A practical workflow is:

1. Open SCWiki before a mining or crafting session.
2. Update tracked material quantities and quality.
3. Use the Scanner view when you need a quick RS reference.
4. Add the blueprints you intend to make to the Planner.
5. Enter the number of output units you want.
6. Use the planner's `NEED` and `QUALITY` results as your material shopping/mining list.
7. Update your tracked inventory as you acquire materials.
8. Reopen the planner to see when the recipe becomes `READY`.

## 16. Building for low resource usage

SCWiki is intentionally implemented as a small native Go terminal application. It has no browser UI, no local web server, no resident database server, and no background sync process.

For the lowest idle overhead, simply run the standalone binary from a normal terminal. No additional service needs to be started.

## Repository

Source code and updates are maintained here:

https://github.com/BailThyRedacted/SCWiki
