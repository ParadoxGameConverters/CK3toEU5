# CK3toEU5

Converts a Crusader Kings III save into a playable Europa Universalis V mod, so you can carry your
medieval world into the age of exploration. Part of the [Paradox Game Converters](https://github.com/ParadoxGameConverters)
family, and heavily inspired by the structure of CK3ToEU4.

Point it at your `.ck3` save (ironman works, saves get melted automatically via rakaly), let it run,
and enable the generated mod in the EU5 launcher. Your empire, your rulers, your dynasties, your wars —
all where you left them.

## What gets converted

The map itself: every county your world knows becomes EU5 locations, owned by countries built from
the independent realms of your save. How firmly a realm holds each piece matters: its de jure land
and anything it has culturally absorbed converts as core, its long-held foreign holdings as
integrated possessions, and only land whose de jure crown a rival still wears arrives as fresh
conquest. Rulers' county/duchy claims become cores on foreign land, so everyone starts with the
same grudges they had in CK3.

Rulers come with their skills, personality traits (mapped to EU5 ruler traits), consorts, heirs and
full family trees — dead relatives included, so the dynasty view has actual history in it. Their
knights and councillors arrive as courtiers, knights carrying a general trait earned from their
martial traits. Child rulers start under a regency. Dynasties keep their names and homes, and the
ruling house's coat of arms is exported as the country flag, copying CK3 emblem textures into the
mod when EU5 doesn't ship them.

Everyone who held the title before shows up too. Past holders become closed ruler terms, so the
dynasty arrives having ruled for generations, the current monarch is numbered against his
namesakes, and the tally is handed to EU5 so the heir born after conversion is crowned the next of
his name rather than the first. A converted Byzantium starts knowing it has had five emperors
called Leon.

Beyond that:

- Governments map from CK3 government plus the country's exact religion to a matching EU5
  government type, setup template and parliament. Templates carry laws the engine gates on faith —
  `iqta_law` is Muslim-only, `patriarchate_law` Orthodox-only — so the match is religion-precise
  and the game doesn't strip its own laws at startup.
- Crown authority and succession laws translate into societal values and heir selection, with
  cultural ethos and traditions nudging the same values further.
- Technology level comes from how far your culture actually advanced through CK3's eras.
- Court language follows the primary culture, liturgical language follows the faith, and the
  cultures your realm genuinely holds land in become accepted or tolerated.
- Custom and reformed faiths become real EU5 religion definitions instead of collapsing into the
  nearest vanilla one; hybrid and divergent cultures get the same treatment.
- Every ruler gets an AI personality read off their CK3 traits and the size of their realm.
- Alliances convert as alliances, rivals as rivals, ongoing wars into the EU5 war manager. Vassal
  contracts decide what kind of subject each dependency becomes — a tributary, a vassal, a
  dominion, or an appanage when the two houses share blood. One ruler with several crowns keeps
  one country.
- International organizations carry over: the Holy Roman Empire as an `hre` around the converted
  empire, the Catholic Church under whoever inherited the Papacy, an autocephalous patriarchate
  for each eastern communion seated on its historical see, and CK3 house blocs as tribal
  confederations.
- County development and holding buildings feed location development, pop sizes, town/city ranks,
  town rights, and the town economy. Only developed counties found new towns, and only city-grade
  ones get the full guild spread — town density stays near vanilla levels so nobody drowns in
  building upkeep. Guilds, markets and temples belong to your pops, like vanilla; the state only
  owns what states own — its castles, universities, and cathedrals converted from CK3 holy sites.
- Only serious fortifications (high-tier walls, famous unique ones) become forts; baseline curtain
  walls convert to nothing, and vanilla fort placement is stripped from converted land.
- Standing armies are built from the men-at-arms regiments your realms actually maintained,
  mapped by type — armored footmen arrive as heavy infantry, horse archers as light cavalry —
  then scaled down to the professional core a realm keeps under arms in peacetime, sized so the
  starting budget survives it. Levies aren't converted because EU5 raises those from population
  natively, which your converted pops already determine. No navies, because vanilla EU5 starts
  every country with zero ships too.
- Rulers keep their reign: a king crowned twenty years before the save reigns twenty years at
  start, with the ruler traits to show for it. Their personal CK3 gold arrives as the starting
  treasury.
- CK3 artifacts turn into works of art displayed at their owner's capital.
- Countries that now hold the land of vanilla colonizers inherit their exploration ambitions,
  so the age of discovery still happens. Whoever owns Lisbon explores Brazil.
- Everything outside your CK3 map — the Americas, unconverted corners of Africa and Asia —
  keeps its vanilla setup, natives and all. A vanilla country the CK3 map only partly overruns is
  trimmed back to the land it still holds instead of being deleted, so no province is left
  ownerless.
- Localization is written for all eleven EU5 languages, including any locations you renamed in CK3.

A validator runs after every conversion, before the game ever sees the mod. It reports dangling
character and country references, invalid or double-owned locations, unknown setup templates,
cultures, religions and town setups, duplicate buildings, and international organizations pointing
at countries that didn't survive — all into the conversion log.

## Running it

The easiest way is the Fronter GUI (built as part of this project): pick your CK3 and EU5
directories, your save, hit convert. The options are exposed there: shatter empires, vassal
split-off, HRE handling, development import, whether custom cultures and faiths become real
definitions, whether technology comes from CK3 or from vanilla's bookmark, army scale, and
whether the ruler's treasury comes along.

To run headless instead, put a `configuration.txt` next to the executable:

```
CK3directory = "C:\Program Files (x86)\Steam\steamapps\common\Crusader Kings III"
CK3DocDirectory = "C:\Users\you\Documents\Paradox Interactive\Crusader Kings III"
EU5directory = "C:\Program Files (x86)\Steam\steamapps\common\Europa Universalis V"
targetGameModPath = "C:\Users\you\Documents\Paradox Interactive\Europa Universalis V\mod"
SaveGame = "C:\path\to\your\save.ck3"
```

The GUI options have headless equivalents, all optional and shown here at their defaults:

```
dynamic_cultures = yes    # hybrid/divergent CK3 cultures become real EU5 cultures
dynamic_religions = yes   # custom/reformed CK3 faiths become real EU5 religions
tech_source = ck3         # ck3 = from your culture's era, vanilla = from the government template
army_scale = normal       # small | normal | large
treasury_import = yes     # the ruler's personal gold becomes starting treasury
war_import = yes          # active CK3 wars continue in EU5, wargoals and all; no = start at peace
```

Run through Fronter and the finished mod is copied into your EU5 mod folder under the save's name,
with a playset created and activated for it, so you can start a new game straight away. Run headless
and the mod is left in `output/` next to the executable for you to copy across yourself.

Conversion behavior is data-driven: the tables under `data/configurables` control province, tag,
religion, culture, government, trait, law, unit, men-at-arms, subject, building and development
mapping. If you disagree with how something converts, that's the place to change it — no rebuild
needed.

## Building

This project uses CMake and can be built with any modern C++ toolchain that supports C++23, though
Visual Studio is the most used and tested.

Clone the repository, then fetch the submodules:

```
git submodule update --init --recursive
```

There are three CMake configurations: x64 release, x64 debug, and x64 clang-tidy. The last runs a
series of static analysis checks on the code base. On Windows, configure and build from a VS
developer environment:

```
cmake --preset x64-release-windows
cmake --build build/x64-release-windows --target CK3toEU5
```

The executable and its data files end up in `build/Release-Windows/CK3toEU5`. Unit tests build as
the `CK3toEU5Tests` target and run from `build/test/Release-Windows`.

When desired changes have been made, open a pull request on GitHub. A number of automatic checks
will run: a Windows build, a Linux build, a clang-tidy scan, a clang-format check, a check that data
files have properly paired curly braces, a scan via Codacy, and a scan via CodeFactor. When those
have passed and one of the code owners has approved the PR, it can be merged.

## Known limitations

- Truces don't convert — CK3 saves don't store them in a form that can be extracted.
- Generated religions for very obscure custom faiths guess their religion group from the local
  dominant religion, which occasionally lands oddly.
- EU5 ships no version number, so the converter checks the game's data layout instead and refuses
  to run against an install that has moved the files it reads. The build checksum is logged for
  bug reports.
- Monarch names EU5 doesn't recognize are left out of the regnal tally, so a ruler with an exotic
  name starts his line over at the first of his name.

## Credits

Built on [commonItems](https://github.com/ParadoxGameConverters/commonItems),
[Fronter](https://github.com/ParadoxGameConverters/Fronter) and
[rakaly](https://github.com/rakaly) for ironman melting, like its sibling converters.
