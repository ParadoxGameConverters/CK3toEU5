# EU5 In-Game Validation

This project now emits a debug-mode validation harness for startup forces and converted wars.

## What the generated mod writes

When the converted world has wars and/or startup forces, the output mod includes:

- `main_menu/setup/start/10_wars.txt`
- `in_game/common/on_action/zz_ck3eu5_startup.txt`
- `in_game/events/zz_ck3eu5_startup.txt`

The startup files do two things:

1. define opening wars in `war_manager`
2. in debug mode, emit validation breadcrumbs to EU5 logs while startup forces are spawned

## Expected validation breadcrumbs

Look for these tokens in EU5 debug-mode logs:

- `CK3EU5_VALIDATE_ON_GAME_START`
- `CK3EU5_VALIDATE_WAR_PRESENT`
- `CK3EU5_VALIDATE_WAR_PARTICIPANT`
- `CK3EU5_VALIDATE_COUNTRY_<TAG>_START`
- `CK3EU5_VALIDATE_ARMY_PRESENT_<TAG>`
- `CK3EU5_VALIDATE_NAVY_PRESENT_<TAG>`
- `CK3EU5_VALIDATE_WAR_LINK_<TAG>_<OPPONENT>`

If something fails to spawn, the mod emits:

- `CK3EU5_VALIDATE_ARMY_MISSING_<TAG>`
- `CK3EU5_VALIDATE_NAVY_MISSING_<TAG>`
- `CK3EU5_VALIDATE_WAR_LINK_MISSING_<TAG>_<OPPONENT>`

## Log files

On this machine, the useful files are:

- `C:\Users\aravi\OneDrive\Documents\Paradox Interactive\Europa Universalis V\logs\debug.log`
- `C:\Users\aravi\OneDrive\Documents\Paradox Interactive\Europa Universalis V\logs\game_tests.log`
- `C:\Users\aravi\OneDrive\Documents\Paradox Interactive\Europa Universalis V\logs\error.log`

`debug.log` is the main source for `debug_log` / `debug_log_scopes`.

`game_tests.log` is the main source for `test_log`.

`error.log` should stay free of `CK3EU5_VALIDATE_*_MISSING_*`.

## Validation flow

1. Launch EU5 in `-debug_mode`.
2. Load a game using the generated converter validation mod.
3. Let the game finish entering the playable map.
4. Quit once.
5. Check the log files above for the validation tokens.

## Interpreting results

Success looks like:

- `CK3EU5_VALIDATE_ON_GAME_START`
- one or more `CK3EU5_VALIDATE_WAR_PRESENT`
- one or more `CK3EU5_VALIDATE_COUNTRY_*_START`
- `CK3EU5_VALIDATE_ARMY_PRESENT_*` for countries expected to have armies
- `CK3EU5_VALIDATE_NAVY_PRESENT_*` for coastal countries expected to have navies
- `CK3EU5_VALIDATE_WAR_LINK_*_*` for converted country pairs expected to start at war
- no `CK3EU5_VALIDATE_*_MISSING_*` tokens in `error.log`

`CK3EU5_VALIDATE_WAR_PRESENT` only proves that some wars existed globally at startup.

`CK3EU5_VALIDATE_WAR_LINK_<TAG>_<OPPONENT>` is the stronger signal that the converted country pair was actually instantiated at war in the running game.
