# Architecture Notes

## Core idea

The most important architectural choice in this codebase is that it treats the converter as a **pipeline of explicit stages** instead of as a giant destination-world constructor.

```text
Configuration
  -> CK3 Input Reader / Importer
  -> EU5 Framework Builder
  -> Mapper Bundle Builder
  -> World Converter
  -> EU5 Outputter
```

That gives you clean seams for testing and later expansion.

---

## `config/`

### `Configuration`

A semantic object that carries:

- source path
- output path
- all mapper file paths
- output mod metadata
- feature toggles and thresholds

### `ConfigurationLoader`

Parses the simple `key = value` config file and resolves relative paths **relative to the config file itself**.

That path-resolution behavior is intentional because it makes sample configs portable.

---

## `common/`

### `PdsParser` and `PdsNode`

A tiny recursive-descent parser for normalized Clausewitz/PDS-style text.

This parser is deliberately limited and should not be mistaken for a full CK3 save parser.

Its job is only to support the normalized input layer.

### `CsvReader`

Simple header-based CSV loader used for mapper bundles and target framework CSV files.

### `filesystem_utils`

Handles reading, writing, directory recreation, UTF-8 BOM / no-BOM writing, and placeholder thumbnails.

### `Logger`

Very small logging utility used by the executable and pipeline.

### `process`

Used to support the `preprocessor_command` bridge.

---

## `ck3/`

### `World`

A normalized CK3-side domain model.

Important property: it contains **domain data**, not parsing behavior.

### `InputReader`

Either:

- reads a normalized input file directly, or
- executes the configured preprocessor command and captures stdout

### `WorldImporter`

Parses normalized source text into `ck3::World` and backfills relationships:

- title ownership
- held titles
- primary titles
- county owners
- top lieges

That backfill step matters because it lets you keep the normalized format simpler.

When `ck3_game_path` is set, the importer also runs an installed-data enrichment pass after parsing. That stage backfills live CK3 metadata such as localized title names, installed de jure hierarchy, county barony province ids, culture pillars, and faith religion-family data without forcing the normalized save format to duplicate vanilla game data.

### `RealmNormalizer`

A dedicated post-import source-world pass that repairs and normalizes CK3 realm state before conversion.

It currently:

- rebuilds holder and liege relationships from the title graph
- recomputes de facto county ownership caches
- fills title and county capitals from normalized barony data
- resolves character primary titles and lieges
- propagates title-level claimant data into character claims
- cleans succession lists such as heirs, claimants, electors, and previous holders

This keeps realm-state repair out of the parser and makes the source-world transformation independently testable.

---

## `eu5/`

### `WorldFramework`

Represents mostly static target-side facts.

Right now it holds:

- location definitions
- province → location reverse lookup
- country color definitions

That is the seed of a much larger future target framework.

### `WorldFrameworkBuilder`

Loads framework data from CSV.

In a production converter this would eventually be replaced or supplemented by extracted real EU5 data.

---

## `mappers/`

### `MapperBundle`

A single object holding all active converter mappings:

- province mappings
- title mappings
- culture mappings
- religion mappings
- government mappings
- reserved tags

Treating this as one bundle rather than a loose collection of helpers makes it much easier to pass around a coherent conversion rule set.

### `MapperBundleBuilder`

Loads mapper files from CSV.

In a more advanced version you would add:

- layered mapper loading
- override precedence
- mod-specific bundles
- version checks
- validation reports

---

## `convert/`

### `TagGenerator`

Resolves explicit tags first and then generates deterministic 3-letter tags when needed.

### `PopSynthesizer`

Converts CK3 county conditions into a rough EU5 pop composition.

This is where you would later grow a much more sophisticated demographic model.

### `WorldConverter`

This is the heart of the prototype.

Its major responsibilities are:

1. identify which CK3 rulers should become EU5 countries
2. assign counties to those countries
3. create converted countries, dynasties, and characters
4. map counties to EU5 locations
5. synthesize location populations and development
6. infer country culture/religion summaries
7. generate subject relations and road connections

The converter deliberately outputs a clean `eu5::World` object instead of writing files directly.

That separation is critical.

---

## `output/`

### `Eu5Outputter`

Takes a fully converted `eu5::World` and writes it into an EU5 mod folder.

Its responsibilities include:

- folder scaffolding
- metadata
- start setup managers
- country definitions
- town setups
- localization
- diagnostics
- debug snapshots

This means the output layer is a real subsystem rather than a few ad-hoc `ofstream` calls sprinkled through the converter.

---

## `diagnostics/`

### `DiagnosticsReport`

Collects:

- info messages
- warnings
- errors

This gives the pipeline a way to proceed while still surfacing issues.

In a more mature version you would likely add:

- categories
- source locations
- severity thresholds
- machine-readable output
- summary stats by mapper / subsystem

---

## Recommended future additions

### Validation subsystem

Add a dedicated validation stage between mapper loading and conversion.

### Real-save normalization subsystem

Create a standalone tool or library layer that normalizes raw CK3 saves into the simplified importer format.

### Rich EU5 framework extraction

Build a real extracted framework from current EU5 files so the converter stops depending on a hand-written sample CSV.

### Output templating

Once the output surface gets large enough, consider moving repetitive output to templates.

### Test expansion

Add tests for:

- parser edge cases
- title selection rules
- tag generation collisions
- pop synthesis behavior
- country inference
- output syntax
