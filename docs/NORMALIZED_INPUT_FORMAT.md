# Normalized CK3 Input Format

The prototype importer accepts a **normalized Clausewitz/PDS-like text file**.

This is not the full CK3 save format.

The goal of the normalized format is to act as a stable intermediate representation between:

- raw CK3 saves and extraction tools
- the converter core

---

## Top-level shape

A file may either:

- contain the world data directly, or
- wrap it inside `world = { ... }`

Example:

```text
world = {
  date = 1337.1.1
  dynasties = { ... }
  dynasty_houses = { ... }
  cultures = { ... }
  faiths = { ... }
  characters = { ... }
  titles = { ... }
  wars = { ... }
  counties = { ... }
}
```

---

## Supported top-level keys

- `date`
- `dynasties`
- `dynasty_houses`
- `cultures`
- `faiths`
- `characters`
- `titles`
- `wars`
- `counties`

---

## `dynasties`

Dynasties are keyed by source dynasty id.

Example:

```text
dynasties = {
  900 = {
    key = "dynasty_wessex"
    good_for_realm_name = yes
  }
}
```

### Supported dynasty keys

- `key`
- `good_for_realm_name`

---

## `dynasty_houses`

Dynasty houses are keyed by source house id.

Example:

```text
dynasty_houses = {
  6054 = {
    key = "house_wessex"
    name = "Wessex"
    localized_name = "Wessex"
    prefix = "de"
    dynasty_id = 900
    house_head_id = 1
  }
}
```

### Supported dynasty-house keys

- `key`
- `name`
- `display_name`
- `localized_name`
- `prefix`
- `dynasty_id` or `dynasty`
- `house_head_id` or `head_of_house`

---

## `cultures`

Cultures are keyed by source culture id.

Example:

```text
cultures = {
  27 = {
    key = anglo_saxon
    display_name = "Anglo Saxon"
  }
}
```

### Supported culture keys

- `key`
- `template`
- `display_name`
- `name`

---

## `faiths`

Faiths are keyed by source faith id.

Example:

```text
faiths = {
  10 = {
    key = catholic
    religion = christianity_religion
    display_name = "Catholic"
  }
}
```

### Supported faith keys

- `key`
- `template`
- `religion`
- `display_name`
- `name`

---

## `characters`

Characters are keyed by character id.

Example:

```text
characters = {
  1 = {
    first_name = Edward
    last_name = Plantagenet
    dynasty_house_id = 6054
    dynasty = Plantagenet
    culture_id = 27
    culture = english
    faith_id = 10
    faith = catholic
    government = feudal
    primary_title = k_england
    liege = 0
    employer = 17
    spouse = 2
    suzerain = 99
    dead = no
    realm_capital_province = 100
    realm_current_strength = 6400
    realm_max_strength = 7100
    realm_levy = 5200
    claims = { d_normandy }
    domain_titles = { k_england c_middlesex }
    gold = 450
    adm = 62
    dip = 58
    mil = 61
    female = no
    birth_date = 1312.11.13
    death_date = 0.0.0
  }
}
```

### Supported character keys

- `first_name` or `name`
- `last_name`
- `dynasty_house_id`
- `dynasty`
- `culture_id`
- `culture`
- `faith_id`
- `faith` or `religion`
- `government`
- `primary_title`
- `liege`
- `employer` or `employer_id`
- `spouse` or `spouse_id`
- `suzerain` or `suzerain_id`
- `dead`
- `realm_capital_province`
- `realm_current_strength`
- `realm_max_strength`
- `realm_levy`
- `claims = { ... }`
- `domain_titles = { ... }`
- `gold`
- `adm`
- `dip`
- `mil`
- `female`
- `birth_date`
- `death_date`

---

## `titles`

Titles are keyed by CK3 title key.

Example:

```text
titles = {
  k_england = {
    source_id = 1
    rank = kingdom
    holder = 1
    de_jure_liege_title = e_britannia
    de_facto_liege_title = e_britannia
    government = feudal
    capital_county = c_middlesex
    capital_province = 100
    de_jure_vassals = { d_essex }
    de_facto_vassals = { d_essex }
    owned_de_jure_counties = { c_middlesex }
    owned_de_facto_counties = { c_middlesex }
    heirs = { 2 3 }
    claimants = { 4 5 }
    electors = { 1 2 4 }
    previous_holders = { 8 9 }
    display_name = "England"
  }
}
```

### Supported title keys

- `source_id`
- `rank`
- `holder` or `holder_id`
- `de_jure_liege_title`, `liege_title`, or `de_jure_liege`
- `de_facto_liege_title` or `de_facto_liege`
- `government`
- `capital_county`
- `capital_province`
- `de_jure_vassals = { ... }`
- `de_facto_vassals = { ... }`
- `owned_de_jure_counties = { ... }`
- `owned_de_facto_counties = { ... }`
- `heirs = { ... }`
- `claimants = { ... }`
- `electors = { ... }`
- `previous_holders = { ... }`
- `capital_barony`
- `display_name` or `name`

If `rank` is omitted, the importer infers it from the title prefix:

- `b_`
- `c_`
- `d_`
- `k_`
- `e_`

---

## `wars`

Wars are keyed by source war id.

Example:

```text
wars = {
  77 = {
    name = "Essex Claim"
    cb_type = claimant_war
    start_date = 1066.9.15
    attacker = 3
    defender = 1
    claimant = 3
    targeted_titles = { c_middlesex }
    attacker = {
      participants = {
        {
          character = 3
          date = 1066.9.15
          contribution_score = 16
        }
      }
    }
    defender = {
      participants = {
        {
          character = 1
          date = 1066.9.15
          contribution_score = 18
        }
      }
    }
  }
}
```

### Supported war keys

- `name`
- `cb_type`
- `start_date`
- `attacker`
- `defender`
- `claimant`
- `targeted_titles = { ... }`
- `attacker = { participants = { ... } }`
- `defender = { participants = { ... } }`

### Supported war participant keys

- `character`
- `date`
- `contribution_score`
- `contribution = { ... }`

The importer accepts both direct `contribution_score` values and raw contribution blocks from normalized raw-save output.

---

## `counties`

Counties are keyed by CK3 county key.

Example:

```text
counties = {
  c_middlesex = {
    source_title_id = 2
    owner = 1
    top_liege = 1
    culture_id = 27
    culture = english
    faith_id = 10
    faith = catholic
    government = feudal
    province = london_province
    display_name = "Middlesex"
    development = 16
    holdings = { castle city temple }
    baronies = { b_london b_westminster }
    barony_display_names = { "London" "Westminster" }
    barony_provinces = { 100 101 }
    neighbors = { c_oxfordshire }
  }
}
```

### Supported county keys

- `source_title_id`
- `owner` or `holder`
- `top_liege`
- `culture_id`
- `culture`
- `faith_id`
- `faith` or `religion`
- `government`
- `province` or `province_key`
- `display_name` or `name`
- `terrain`
- `development`
- `holdings = { ... }`
- `baronies = { ... }`
- `barony_display_names = { ... }`
- `barony_provinces = { ... }`
- `neighbors = { ... }`

---

## Backfill behavior

The importer performs several repair / backfill steps:

- if county-rank titles exist without county entries, counties are synthesized
- if culture / faith / dynasty-house ids are present, names / keys are resolved from the top-level lookup tables
- if title graph summaries are omitted, de-facto vassals and owned de-jure / de-facto county sets are computed from the liege graph
- characters gain held titles from title ownership
- characters gain primary titles if one was not specified explicitly
- title claimants are propagated into character claim lists when needed
- counties inherit owners from matching county titles if needed
- counties inherit top lieges by walking the character liege chain if needed
- counties can inherit barony keys, names, and province ids from title de jure vassals if county entries omit them

This makes the normalized format more forgiving.

---

## Why keep this format

Do not think of this normalized input layer as a temporary annoyance.

It is useful because it lets you:

- test the converter without real saves
- build deterministic fixtures
- keep raw-save extraction separate from conversion logic
- evolve the importer independently from the rest of the pipeline

That separation will save a lot of pain later.
