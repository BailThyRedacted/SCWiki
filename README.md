# SCWiki

A deliberately small local TUI wiki/tracker for Star Citizen mining and crafting data.

## Features

- Material browser with mining scanner RS signatures.
- Local material inventory with quantity and 0–1000 quality.
- Blueprint browser and tracking.
- Scanner RS lookup with likely material and node count.
- **Crafting planner** with persistent blueprint craft quantities.
- Planner material roll-up: required, owned, quantity deficit, quality, and minimum quality.
- No network connection required at runtime.
- Single small Go binary; data is stored as JSON.

## Controls

`1` Materials  `2` Blueprints  `3` Scanner  `4` Tracked  `5` Planner

`j/k` move  `/` filter  `t` plan  `a` crafts  `x` remove  `r` scanner lookup  `s` save  `q` quit

In the Crafting Planner, `t` adds/removes the selected blueprint. `a` prompts for the number of output units to plan. `0` removes it. The planner calculates material requirements from the blueprint recipe and output quantity, then compares them with tracked inventory.

Quality is evaluated separately from quantity. A material is marked `QUALITY` when tracked inventory quality is below the highest minimum-quality requirement among planned blueprints using that material.

## Data

Default data path: `~/.local/share/scwiki/wiki.json`

Override with `SCWIKI_DATA=/path/to/data ./scwiki`.
