"""
Compares the freshly converted mod against vanilla's 1337 start.

The comparison covers the numbers the economy calibration was aimed at:
urban footprint, standing armies, accepted cultures and treasuries.
"""
import re
import os
import sys
import collections

OUTPUT = r'build\Release-Windows\CK3toEU5\output'
MOD = sys.argv[1] if len(sys.argv) > 1 else max(
    (os.path.join(OUTPUT, name) for name in os.listdir(OUTPUT)), key=os.path.getmtime)
GAME = r'C:\Program Files (x86)\Steam\steamapps\common\Europa Universalis V\game'
VANILLA = os.path.join(GAME, r'main_menu\setup\start')
print('checking', MOD)


def read(path):
    with open(path, encoding='utf-8-sig', errors='replace') as handle:
        return handle.read()


def blocks_at(text, want_depth):
    """Yields (name, body) for every named block sitting at the given brace depth."""
    depth = 0
    open_at = {}
    pending = ''
    for index, char in enumerate(text):
        if char == '{':
            depth += 1
            open_at[depth] = (pending.strip().split('=')[0].strip().split()[-1] if pending.strip() else '', index + 1)
            pending = ''
        elif char == '}':
            if depth == want_depth:
                name, start = open_at.get(depth, ('', index))
                if name:
                    yield name, text[start:index]
            depth -= 1
            pending = ''
        else:
            pending += char


def top_blocks(text):
    return blocks_at(text, 2)


def urban(path):
    text = read(path)
    towns = 0
    cities = 0
    for _, body in top_blocks(text):
        rank = re.search(r'\brank\s*=\s*(\w+)', body)
        if not rank:
            continue
        towns += 1
        if rank.group(1) == 'city':
            cities += 1
    return towns, cities


print('== urban footprint ==')
van_towns, van_cities = urban(os.path.join(VANILLA, '07_cities_and_buildings.txt'))
our_towns, our_cities = urban(os.path.join(MOD, r'main_menu\setup\start\07_cities_and_buildings.txt'))
print(f'vanilla urban {van_towns} of which cities {van_cities}')
print(f'ours    urban {our_towns} of which cities {our_cities}')

print()
print('== countries ==')
for label, path in (('vanilla', os.path.join(VANILLA, '10_countries.txt')),
                    ('ours', os.path.join(MOD, r'main_menu\setup\start\10_countries.txt'))):
    text = read(path)
    tags = 0
    owned = 0
    regiments = 0
    accepted = 0
    tolerated = 0
    over_capacity = []
    treasuries = 0
    no_treasury = []
    for tag, body in blocks_at(text, 3):
        if len(tag) != 3 or not tag.isupper():
            continue
        tags += 1
        for kind in ('own_control_core', 'own_control_integrated', 'own_control_conquered', 'own_core'):
            for match in re.finditer(kind + r'\s*=\s*\{([^}]*)\}', body):
                if kind != 'own_core':
                    owned += len(match.group(1).split())
        regiments += len(re.findall(r'\bsub_units\s*=', body))
        for match in re.finditer(r'accepted_cultures\s*=\s*\{([^}]*)\}', body):
            accepted += len(match.group(1).split())
        for match in re.finditer(r'tolerated_cultures\s*=\s*\{([^}]*)\}', body):
            tolerated += len(match.group(1).split())
        if 'currency_data' in body or re.search(r'\bgold\s*=', body):
            treasuries += 1
        else:
            no_treasury.append(tag)
    print(f'{label}: {tags} tags, {owned} owned locations, {regiments} standing regiments, '
          f'{accepted} accepted cultures, {tolerated} tolerated, {treasuries} with a treasury')
    if label == 'ours':
        print(f'  countries with no treasury: {len(no_treasury)}')

print()
print('== ownership and definitions ==')
countries_text = read(os.path.join(MOD, r'main_menu\setup\start\10_countries.txt'))
owner_of = {}
duplicates = []
started = set()
claims = 0
for tag, body in blocks_at(countries_text, 3):
    if len(tag) != 3 or not tag.isupper():
        continue
    started.add(tag)
    # own_core counts: it means owned-but-occupied, not a claim. Claims live in
    # our_cores_conquered_by_others and stay out of the ownership map on purpose.
    for kind in ('own_control_core', 'own_control_integrated', 'own_control_conquered', 'own_core'):
        for match in re.finditer(r'\b' + kind + r'\s*=\s*\{([^}]*)\}', body):
            for location in match.group(1).split():
                if location in owner_of:
                    duplicates.append((location, owner_of[location], tag))
                owner_of[location] = tag
    for match in re.finditer(r'our_cores_conquered_by_others\s*=\s*\{([^}]*)\}', body):
        claims += len(match.group(1).split())
print(f'{len(owner_of)} locations owned, {len(duplicates)} owned twice, {claims} foreign core claims')
if duplicates:
    print('  ', duplicates[:8])

defined = {}
for name in os.listdir(os.path.join(MOD, r'in_game\setup\countries')):
    text = read(os.path.join(MOD, r'in_game\setup\countries', name))
    for tag, body in blocks_at('root = {\n' + text + '\n}', 2):
        if len(tag) == 3 and tag.isupper():
            defined[tag] = body
historic = {tag for tag, body in defined.items() if re.search(r'is_historic\s*=\s*yes', body)}
cultured = {tag for tag, body in defined.items() if 'culture_definition' in body}
unstarted = (cultured - started) - historic
print(f'{len(defined)} tags defined, {len(historic)} historic, {len(unstarted)} real countries with no start and no historic flag')
if unstarted:
    print('  ', sorted(unstarted)[:12])

print()
print('== ports ==')
sea_side = set()
with open(r'C:\Program Files (x86)\Steam\steamapps\common\Europa Universalis V\game\in_game\map_data\ports.csv',
          encoding='utf-8-sig', errors='replace') as handle:
    for line in handle:
        parts = line.strip().split(';')
        if len(parts) >= 2 and parts[0] and not parts[0].startswith('#'):
            sea_side.add(parts[0].strip())
setups = {}
for path in (os.path.join(MOD, r'in_game\common\town_setups\zzz_converted_town_setups.txt'),):
    for name, body in blocks_at('root = {\n' + read(path) + '\n}', 2):
        setups[name] = body
port_buildings = ('wharf', 'dock', 'shipyard', 'drydock')
offenders = []
for location, body in blocks_at(read(os.path.join(MOD, r'main_menu\setup\start\07_cities_and_buildings.txt')), 2):
    setup = re.search(r'setup\s*=\s*(\w+)', body)
    if not setup or location in sea_side:
        continue
    text = setups.get(setup.group(1), '')
    for building in port_buildings:
        if re.search(r'\b' + building + r'\b', text) or re.search(r'\b' + building + r'\b', body):
            offenders.append((location, setup.group(1), building))
print(f'{len(setups)} converted town setups, {len(offenders)} landlocked towns carrying a port building')
if offenders:
    print('  ', offenders[:8])

print()
print('== our own files, parse hazards ==')
bad = 0
for root, _, files in os.walk(MOD):
    for name in files:
        if not name.endswith(('.txt', '.yml')):
            continue
        path = os.path.join(root, name)
        text = read(path)
        if 'suppress_no_pops_error' in text:
            print('  suppress_no_pops_error still in', name)
            bad += 1
        if name.endswith('.yml'):
            for number, line in enumerate(text.split('\n')[1:], start=2):
                if not line.strip():
                    continue
                if not re.match(r'^ [\w.\-]+:\d* ".*"\s*$', line):
                    print(f'  malformed loc line {name}:{number}: {line[:120]!r}')
                    bad += 1
print(f'  {bad} hazards found')

print()
print('== languages ==')
# The game only resolves language references to leaves: a dialect, or a language without dialects,
# or a language listing itself among its own dialects (greek_language). Anything else fails to
# parse and takes the surrounding block down with it.
def balanced_body(text, start):
    depth = 1
    for index in range(start, len(text)):
        if text[index] == '{':
            depth += 1
        elif text[index] == '}':
            depth -= 1
            if depth == 0:
                return text[start:index]
    return text[start:]


language_dialects = {}
for name in os.listdir(os.path.join(GAME, r'in_game\common\languages')):
    if not name.endswith('.txt'):
        continue
    text = re.sub(r'#[^\n]*', '', read(os.path.join(GAME, r'in_game\common\languages', name)))
    for lang, body in blocks_at('root = {\n' + text + '\n}', 2):
        dialects = re.search(r'dialects\s*=\s*\{', body)
        language_dialects[lang] = []
        if dialects:
            language_dialects[lang] = re.findall(r'(\w+)\s*=\s*\{', balanced_body(body, dialects.end()))
valid_leaves = set()
for lang, dialects in language_dialects.items():
    for dialect in dialects:
        valid_leaves.add(dialect)
    if not dialects or lang in dialects:
        valid_leaves.add(lang)
bad_languages = []
for rel in (r'main_menu\setup\start\10_countries.txt',
            r'in_game\common\cultures\zzz_converted_cultures.txt',
            r'in_game\common\religions\zzz_converted_religions.txt'):
    path = os.path.join(MOD, rel)
    if not os.path.exists(path):
        continue
    for match in re.finditer(r'\b(court_language|liturgical_language|language)\s*=\s*(\w+)', read(path)):
        if match.group(2) not in valid_leaves:
            bad_languages.append((os.path.basename(rel), match.group(1), match.group(2)))
print(f'{len(valid_leaves)} referenceable language leaves, {len(bad_languages)} invalid references in the mod')
if bad_languages:
    print('  ', sorted(set(bad_languages))[:10])

print()
print('== wars ==')
wars_text = read(os.path.join(MOD, r'main_menu\setup\start\16_wars.txt'))
war_count = len(re.findall(r'(?m)^\twar\s*=\s*\{', wars_text))
goals = re.findall(r'take_province\s*=\s*\{[^}]*location\s*=\s*(\w+)', wars_text)
goal_owned = sum(1 for goal in goals if goal in owner_of)
print(f'{war_count} wars, {len(goals)} with a wargoal, {goal_owned} goals on owned land')
if war_count != len(goals):
    print('  WARS WITHOUT GOALS - the game cannot score or end them')

print()
print('== localization hygiene ==')
suspicious = []
for name in os.listdir(os.path.join(MOD, r'main_menu\localization\english')):
    if not name.endswith('.yml'):
        continue
    text = read(os.path.join(MOD, r'main_menu\localization\english', name))
    for match in re.finditer(r'^\s*(\S+?):\d*\s+"(.*)"\s*$', text, re.M):
        key, value = match.groups()
        if re.search(r'ONCLICK|TOOLTIP|[\x00-\x1f]|\$[\w.|]+\$', value):
            suspicious.append((key, value[:80]))
print(f'{len(suspicious)} loc values still carrying markup or unresolved references')
for key, value in suspicious[:8]:
    print(f'  {key}: {value!r}')

print()
print('== art ==')
art = read(os.path.join(MOD, r'main_menu\setup\start\11_art.txt'))
keys = re.findall(r'key\s*=\s*(conv_art_\d+)', art)
dates = collections.Counter(re.findall(r'creation_date\s*=\s*(\d+)\.', art))
loc = read(os.path.join(MOD, r'main_menu\localization\english\zzz_converted_l_english.yml'))
described = set(re.findall(r'^ (conv_art_\d+)_desc: "(.+)"$', loc, re.M))
named = set(re.findall(r'^ (conv_art_\d+): "', loc, re.M))
print(f'{len(keys)} works of art, {len(named)} named, {len(described)} described')
missing = [key for key in keys if key not in {k for k, _ in described}]
print(f'missing descriptions: {len(missing)}')
print('creation years spread:', sorted(dates.items())[:4], '...', sorted(dates.items())[-4:])
for key, text in sorted(described)[:5]:
    print(f'  {key}: {text[:160]}')
