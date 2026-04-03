# CK3 → EU5 Converter Prototype

This repository is a **clean-room starter codebase** for a future **Crusader Kings 3 → Europa Universalis 5** converter.

It is not a finished converter yet.

What it **does** already:

- builds a normalized CK3 world importer
- can normalize raw `.ck3` saves into that snapshot format with a built-in Rakaly-based bridge
- builds an EU5 framework loader
- can extract live EU5 framework CSVs from an installed EU5 copy
- builds a mapper bundle loader
- includes deterministic county auto-matching fallbacks against live EU5 location keys / province definitions
- can validate generated cultures / religions / governments / country ranks / pop types against a live EU5 install
- converts a CK3 world into an intermediate EU5 world model
- imports active CK3 wars into an EU5 war model
- writes startup spawn scripts for literal opening armies and navies
- writes a valid-looking EU5 mod folder skeleton
- writes `setup/start` managers, `setup/countries`, `town_setups`, localization, and diagnostics
- includes sample input, sample mappings, a sample generated mod, and a smoke test

What it **does not** do yet:

- understand the full CK3 save schema; the built-in normalizer is intentionally first-pass and heuristic
- generate dynamic EU5 religions, cultures, laws, estates, or complete economic setup
- import every CK3 edge case, including full claims history, cadet-branch depth, artifacts, men-at-arms composition, or building detail
- validate the generated EU5 mod against the live shipped game files automatically

So think of this repo as the **best possible high-quality prototype skeleton**, not as the final production converter.

---

## Why this codebase exists

A CK3 → EU5 converter is harder than a traditional “tag and province” converter.

EU5 setup is much richer than older EU games, so a serious converter has to do all of these things well:

1. import a complicated feudal / dynastic source world
2. normalize that source world into stable internal data structures
3. load a static target-game framework
4. load configurable mapping bundles
5. synthesize a believable target world
6. output a mod in the exact file and encoding layout EU5 expects

This repository is built around that exact pipeline.

---

## Design philosophy

The architecture intentionally keeps these layers separate:

- **configuration**
- **source import**
- **target framework loading**
- **mapper bundle loading**
- **world conversion**
- **output**
- **diagnostics**

That separation is the single most important structural decision in the whole project.

It prevents the classic converter failure mode where a single giant `World.cpp` file becomes:

- parser
- data model
- business logic
- heuristics engine
- output generator
- error logger

all at once.

---

## Repository layout

```text
src/
  ck3/
    input_reader.*
    raw_save_normalizer.*
    world.*
    world_importer.*
  common/
    csv_reader.*
    filesystem_utils.*
    logger.*
    pds_node.*
    pds_parser.*
    process.*
    string_utils.*
  config/
    configuration.*
    configuration_loader.*
  convert/
    pop_synthesizer.*
    tag_generator.*
    world_converter.*
  diagnostics/
    diagnostics_report.*
  eu5/
    framework.*
    framework_builder.*
    world.h
  mappers/
    mapper_bundle.*
  output/
    eu5_outputter.*
  main.cpp
  normalizer_main.cpp
  extract_eu5_main.cpp

data/configurables/
  location_framework.csv
  province_mappings.csv
  title_mappings.csv
  culture_mappings.csv
  religion_mappings.csv
  government_mappings.csv
  country_colors.csv

examples/
  sample_ck3_world.pds
  sample_config.cfg
  generated/sample_mod/

tests/
  smoke_test.cpp

docs/
  ARCHITECTURE.md
  NORMALIZED_INPUT_FORMAT.md
```

---

## End-to-end pipeline

The executable performs these steps:

1. load config
2. validate required paths and settings
3. read CK3 input
4. parse normalized CK3 snapshot into `ck3::World`
5. load EU5 location / color framework from CSV
6. load all mapper bundles from CSV
7. convert into `eu5::World`
8. write an EU5 mod folder
9. emit diagnostics and debug snapshots

Conceptually:

```text
Configuration
  -> CK3 World Importer
  -> EU5 Framework Builder
  -> Mapper Bundle Builder
  -> World Converter
  -> EU5 Outputter
```

---

## Extracting Live EU5 Framework Data

The prototype still keeps its framework layer CSV-driven on purpose, but the repo now includes a helper executable that can regenerate those CSVs from a local EU5 install:

```text
ck3_to_eu5_extract_eu5_framework "C:\Program Files (x86)\Steam\steamapps\common\Europa Universalis V\game" "<output-directory>"
```

It currently extracts:

- `location_framework.csv`
- `country_colors.csv`

The extractor reads the shipped EU5 files for:

- named locations
- province / area / region hierarchy
- raw goods, climate, and topography
- town / city setup hints
- coastal ports
- named map colors and country color definitions

That keeps the actual converter pipeline unchanged while replacing the prototype’s fake target-data inputs with real shipped EU5 data.

If you also set `eu5_game_path` in the converter config, the main conversion run will validate generated identifiers against the installed game’s shipped definitions and emit diagnostics for unknown cultures, religions, government types, country ranks, or pop types.

---

## Current source-side model

The prototype currently imports a **normalized CK3 world snapshot**, not a raw save.

The normalized model now has eight primary object groups:

- `dynasties`
- `dynasty_houses`
- `cultures`
- `faiths`
- `characters`
- `titles`
- `wars`
- `counties`

### Character fields currently used

- id
- first / last name
- dynasty house id
- dynasty
- culture id
- culture
- faith id
- faith / religion
- government
- primary title
- liege
- employer / spouse / suzerain
- dead state / death date
- realm military strength fields
- claims
- realm capital province
- domain titles
- birth / death date
- gold
- adm / dip / mil
- female

### Title fields currently used

- key
- source id
- rank
- holder
- de jure liege title
- de facto liege title
- de facto vassals
- aggregated owned de jure / de facto counties
- heirs / claimants / electors / previous holders
- government
- capital county
- capital province
- de jure vassals
- display name

### Dynasty / house fields currently used

- dynasty key and `good_for_realm_name`
- house key
- house name / localized name / prefix
- parent dynasty id
- house head id

### County fields currently used

- key
- source title id
- owner
- top liege
- culture id
- culture
- faith id
- faith / religion
- government
- province definition key
- display name
- development
- holdings
- barony keys / names / province ids
- neighbors

### War fields currently used

- source war id
- name
- casus belli type
- start date
- attacker / defender / claimant ids
- targeted titles
- attacker / defender participant lists
- participant join dates and contribution scores

That is deliberately small enough to be stable and testable, while still rich enough to generate a meaningful first EU5 world.

---

## Current target-side model

The converter produces these EU5-side concepts:

- countries
- locations
- pops
- dynasties
- characters
- subject relations
- scripted relations / opinions / rivals
- markets
- building instances
- force plans
- startup forces
- wars
- roads
- localization

### EU5 countries currently include

- tag
- source character / title provenance
- display name / adjective
- capital location
- primary culture / religion
- government type
- country rank
- starting technology level
- currencies
- two government value axes:
  - `centralization_vs_decentralization`
  - `traditionalist_vs_innovative`
- ruler character reference
- owned/core locations
- accepted / tolerated cultures
- discovered regions

### EU5 locations currently include

- owner tag
- rank
- province definition
- region / area
- raw good
- development
- town setup
- institution presence
- synthesized pops

---

## What the heuristics currently do

### Country creation

The converter creates EU5 countries from CK3 characters using these rules:

- all independent direct county holders become countries
- title mappings force country creation when present
- subject realms can also become countries if the config allows it and they are large enough
- title-side heirs / claimants / electors contribute small prestige weight to converted realms

### Tag assignment

Tags are assigned in this order:

1. explicit title mapping tag
2. generated 3-letter tag based on title key
3. synthetic fallback tag if needed

### County assignment

Each CK3 county is assigned to the nearest eligible country character by walking up the liege chain.

### Character carry-over

The converter always creates ruler characters for converted countries.

It also carries over a bounded set of attached notables when they clearly belong to the same converted realm:

- spouses of converted rulers
- title heirs
- title claimants
- title electors
- living previous holders when they still exist in the imported character set
- dead previous holders when they are present in the save, with death dates preserved or conservatively backfilled

### Pop synthesis

The current `PopSynthesizer` is intentionally heuristic.

It derives pop mixes from:

- development
- holdings (`castle`, `city`, `temple`, `tribe`)
- owner culture / faith for elites
- county culture / faith for mass pops

It currently creates combinations of:

- nobles
- clergy
- burghers
- laborers
- soldiers
- peasants or tribesmen

### Culture and religion inference

Country primary culture and religion are inferred from the total weight of pops in owned locations.

Secondary cultures are promoted to:

- **accepted** if population share is at least 25%
- **tolerated** if population share is at least 10%

### Rank inference

Location rank is inferred from county development, city holdings, and the target framework’s default rank.

### Roads

Roads are generated between neighboring counties that end up inside the same converted country.

### Subjects

If a converted country’s ruler has a liege that is also converted into a country, a subject relation is emitted.

Right now the default subject type is `vassal`.

---

## What the outputter writes

The outputter currently creates:

- `.metadata/metadata.json`
- `.metadata/thumbnail.png`
- `main_menu/setup/start/*.txt`
- `in_game/setup/countries/*.txt`
- `in_game/common/town_setups/*.txt`
- `in_game/localization/english/*.yml`
- diagnostics and debug snapshots

### Start setup files written

- `00_institutions.txt`
- `01_dynasties.txt`
- `02_characters.txt`
- `03_locations.txt`
- `04_countries.txt`
- `05_diplomacy.txt`
- `06_markets.txt`
- `07_roads.txt`
- `08_development.txt`
- `09_buildings.txt`
- `10_wars.txt`

### Additional generated content

- country definitions with colors and default culture / religion
- reusable town setup templates
- startup `on_game_start` hooks and hidden events that spawn literal armies / navies
- localization for generated tags, dynasties, characters, and used locations
- CSV / text debug summaries

---

## Configuration file

The configuration format is a very small `key = value` syntax.

See `examples/sample_config.cfg`.

### Required paths

- `ck3_input`
- `output_mod_path`
- `location_framework`
- `province_mappings`
- `title_mappings`
- `culture_mappings`
- `religion_mappings`
- `government_mappings`
- `country_colors`

### Important optional settings

- `ck3_game_path`
- `preprocessor_command`
- `mod_name`
- `mod_id`
- `mod_version`
- `supported_game_version`
- `verbose_logging`
- `write_debug_snapshots`
- `prefer_subject_realms`
- `minimum_subject_counties`
- `default_technology_level`
- `default_gold`

### `ck3_game_path`

If set, the importer will load live metadata from the installed CK3 data files after parsing the normalized save snapshot.

That enrichment currently backfills:

- localized title names and installed de jure title relationships
- county barony lists and barony province ids from landed titles
- culture metadata such as ethos, heritage, language, and parents
- faith metadata such as religion family, religion display name, and installed doctrines
- dynasty-house localization tokens such as `dynn_*`

This keeps raw-save normalization focused on save extraction, while letting the source world pick up cleaner vanilla metadata from the game install.

### `preprocessor_command`

This is the intended bridge to a future real-save pipeline.

If set, the converter will execute the command and read **stdout** as the normalized world snapshot.

The token `{input}` is replaced with the CK3 save path.

That means a future workflow can look like this:

```text
raw CK3 save
  -> rakaly / custom flattener / custom normalizer
  -> normalized snapshot on stdout
  -> this converter
```

That hook is the easiest way to evolve this prototype into a real converter without rewriting the rest of the architecture.

### `auto_normalize_raw_ck3`

When enabled, the converter will detect a `.ck3` input file and run the built-in raw-save normalizer before import.

That normalizer currently extracts:

- living characters
- title ownership and liege structure
- county culture / faith / development
- dynasty-house names
- rough holding mixes

It is meant to get real saves into the pipeline without forcing the rest of the architecture to change.

After import, the CK3 world now also runs through a dedicated realm/succession normalizer that rebuilds primary titles, lieges, title capitals, county top lieges, and propagated claimant data from the source graph before EU5 conversion starts.

---

## CSV data files

The converter is intentionally data-driven wherever practical.

### `location_framework.csv`

Defines the target EU5 location framework used by the converter.

This file is where you describe:

- location keys
- province definitions
- display names
- raw goods
- region / area
- climate / topography
- default rank
- optional preselected town setup
- coastal flag

### `province_mappings.csv`

Maps CK3 county keys to one or more EU5 location keys.

### `title_mappings.csv`

Maps CK3 higher titles to explicit EU5 tags and optionally names, adjectives, rank, and tech level.

### `culture_mappings.csv`

Maps CK3 cultures to EU5 cultures.

### `religion_mappings.csv`

Maps CK3 faiths to EU5 religions.

### `government_mappings.csv`

Maps CK3 government styles to EU5 government types and value-axis defaults.

### `country_colors.csv`

Provides country definition visuals for known tags.

If a tag is missing, the outputter generates deterministic fallback RGB colors.

---

## Building

```bash
cmake -S . -B build
cmake --build build
```

### Run the sample conversion

```bash
./build/ck3_to_eu5 examples/sample_config.cfg
```

### Normalize a real CK3 save

```bash
./build/ck3_to_eu5_normalize /path/to/save.ck3
```

### Run the smoke test

```bash
cd build
ctest --output-on-failure
```

---

## Sample content included

This repository includes:

- a sample normalized CK3 snapshot
- sample mapping CSVs
- a generated sample EU5 mod under `examples/generated/sample_mod`
- a smoke test that builds the pipeline and checks critical outputs

That gives you both:

- a runnable demo
- a concrete reference for how the pipeline is supposed to behave

---

## What is solid already

These parts are already good foundations and should survive into a production converter:

- importer / framework / mapper / converter / output separation
- small normalized domain models
- PDS parser for normalized Clausewitz-like input
- data-driven mapping bundle loading
- deterministic tag generation
- diagnostics subsystem
- EU5 mod folder scaffolding
- tests and sample data

---

## What still needs major work

### 1. Real CK3 save ingestion

The biggest missing layer is a robust real-save normalizer.

You will eventually need:

- save decompression / plaintext handling
- raw CK3 object extraction
- title history and de jure/de facto separation
- dynasties, cadet branches, houses, spouses, heirs, regencies
- cultures, faith doctrines, holy sites, special doctrines
- buildings, development, innovations, MAA, levies, wars, artifacts
- vassal contracts, crown authority, succession laws

### 2. Better geopolitical synthesis

Still missing:

- unions / marches / tributaries / church states / nomad handling
- republic / theocracy / clan / tribal special cases
- realm-shattering or consolidation policies
- de jure repair logic

### 3. EU5 systemic setup

Still missing or simplified:

- laws and policy mapping
- estates
- dynamic religion generation
- dynamic culture generation
- market logic
- institution spread beyond feudalism
- full economic, building, and development calibration
- historical ruler terms / regnal history
- diplomacy beyond subjects
- deeper war-state import beyond the current active-war bootstrap

### 4. Validation

A production version should add validators for:

- county → location coverage
- title → tag conflicts
- invalid cultures / religions / governments
- subject loops
- disconnected road chains
- missing capital mappings
- generated tag collisions with vanilla / DLC / mods
- EU5 syntax and data-key validation against dumped script docs

---

## Recommended path to turn this into a real converter

### Milestone 1 — real source normalization

Build a normalizer that turns raw CK3 saves into the normalized snapshot this prototype already understands.

Do **not** throw away the normalized format. It is a useful stability layer.

### Milestone 2 — enrich the internal CK3 model

Add:

- full dynasty / house structure
- succession data
- title contracts / realm laws
- claims / wars / truces
- holdings and buildings
- innovations and special regional features

### Milestone 3 — enrich the EU5 framework

Move from the tiny sample CSV to extracted real EU5 framework data.

That means building extraction tools or prebuilt datasets for:

- all valid locations
- province definitions
- areas / regions
- location templates / goods
- default country definitions
- valid government / law / culture / religion keys

### Milestone 4 — replace simple heuristics with configurable policies

Examples:

- how duchy vassals become subjects
- how succession maps into ruler / heir structures
- how crown authority maps into law groups
- how development, buildings, and holdings map into EU5 town setups and population

### Milestone 5 — mod and compatibility support

Eventually add:

- source mod filesystem support
- target mod layering support
- per-mod mapper bundles
- version checks and compatibility matrices

---

## Important honesty note

This repository is intentionally honest about its current scope.

It is already a **real codebase** and not just pseudocode.
It builds, runs, tests, and emits a structured EU5 mod folder.

But it is also still a **prototype**.

The right way to use it is:

- keep the architecture
- expand the source normalization layer
- deepen the mapping data
- deepen the EU5 world synthesis
- keep output and diagnostics separate

If you do that, this repo can become the spine of a production CK3 → EU5 converter instead of collapsing into a monolith.

---

## Where to read next

- `docs/ARCHITECTURE.md`
- `docs/NORMALIZED_INPUT_FORMAT.md`
- `examples/sample_config.cfg`
- `examples/sample_ck3_world.pds`
- `tests/smoke_test.cpp`
