"""Checks the cultures the converter generates from CK3's custom, hybrid and divergent cultures:
that everything they reference exists in EU5, that they carry the same fields vanilla cultures do,
that they are localized, and that the pops and countries using them agree on the name."""
import os
import re
import collections

MOD = r'build\Release-Windows\CK3toEU5\output\Malik_al_Muazzam_Danyal_of_the_Yamamoto_Empire_1407_05_20'
GAME = r'C:\Program Files (x86)\Steam\steamapps\common\Europa Universalis V\game'


def read(path):
    with open(path, encoding='utf-8-sig', errors='replace') as handle:
        return handle.read()


def blocks(text, want_depth=0, _depth=0):
    """Yields (name, body) for every named block at the wanted brace depth. Paradox files indent as
    they please, so this walks braces rather than trusting column zero."""
    index = 0
    while index < len(text):
        match = re.compile(r'([\w.]+)\s*=\s*\{').search(text, index)
        if not match:
            return
        depth = 1
        end = match.end()
        while depth and end < len(text):
            if text[end] == '{':
                depth += 1
            elif text[end] == '}':
                depth -= 1
            end += 1
        body = text[match.end():end - 1]
        if _depth == want_depth:
            yield match.group(1), body
        else:
            yield from blocks(body, want_depth, _depth + 1)
        index = end


def gather(folder, depth=0):
    """Every named block at the given depth across a folder's files, so nested definitions such as
    dialects inside their parent language are picked up too."""
    found = {}
    for root, _, files in os.walk(folder):
        for name in files:
            if not name.endswith('.txt'):
                continue
            text = read(os.path.join(root, name))
            for level in range(depth + 1):
                for block, body in blocks(text, level):
                    found.setdefault(block, body)
    return found


vanilla_cultures = gather(os.path.join(GAME, r'in_game\common\cultures'))
languages = gather(os.path.join(GAME, r'in_game\common\languages'), depth=2)
groups = gather(os.path.join(GAME, r'in_game\common\culture_groups'))
ours = gather(os.path.join(MOD, r'in_game\common\cultures'))
print(f'{len(vanilla_cultures)} vanilla cultures, {len(ours)} generated, '
      f'{len(languages)} languages, {len(groups)} culture groups')

overlap = set(ours) & set(vanilla_cultures)
print(f'generated names colliding with vanilla: {len(overlap)} {sorted(overlap)[:5]}')

# Which fields do vanilla cultures carry, and which do ours? Only the culture's own keys count, so
# skip over anything nested (the culture names listed inside an opinions block, for instance).
def top_keys(body):
    keys = set()
    index = 0
    while index < len(body):
        match = re.compile(r'(\w+)\s*=\s*(\{)?').search(body, index)
        if not match:
            break
        keys.add(match.group(1))
        if match.group(2):
            depth = 1
            end = match.end()
            while depth and end < len(body):
                if body[end] == '{':
                    depth += 1
                elif body[end] == '}':
                    depth -= 1
                end += 1
            index = end
        else:
            index = match.end()
    return keys


def fields(definitions):
    counter = collections.Counter()
    for body in definitions.values():
        for key in top_keys(body):
            counter[key] += 1
    return counter


van_fields = fields(vanilla_cultures)
our_fields = fields(ours)
print('\nfield coverage (vanilla / ours):')
for key in sorted(set(van_fields) | set(our_fields)):
    print(f'  {key:<24} {van_fields.get(key, 0):>5} / {our_fields.get(key, 0):>5}')

# Every gfx tag vanilla uses anywhere is fair game; ours must stay inside that set.
known_tags = set()
for body in vanilla_cultures.values():
    for match in re.finditer(r'tags\s*=\s*\{([^}]*)\}', body):
        known_tags.update(match.group(1).split())

problems = collections.Counter()
for name, body in ours.items():
    language = re.search(r'language\s*=\s*(\w+)', body)
    if not language:
        problems['no language'] += 1
    elif language.group(1) not in languages:
        problems[f'unknown language {language.group(1)}'] += 1
    for match in re.finditer(r'culture_groups\s*=\s*\{([^}]*)\}', body):
        for group in match.group(1).split():
            if group not in groups:
                problems[f'unknown culture group {group}'] += 1
    for match in re.finditer(r'tags\s*=\s*\{([^}]*)\}', body):
        for tag in match.group(1).split():
            if tag not in known_tags:
                problems[f'unknown gfx tag {tag}'] += 1
print('\nreference problems:', dict(problems) or 'none')

# Localization: name and adjective keys, the way vanilla does it.
loc = read(os.path.join(MOD, r'main_menu\localization\english\zzz_converted_l_english.yml'))
our_keys = set(re.findall(r'(?m)^ ([\w.]+):', loc))
van_loc = ''
for root, _, files in os.walk(os.path.join(GAME, 'main_menu', 'localization', 'english')):
    for name in files:
        if name.endswith('.yml'):
            van_loc += read(os.path.join(root, name))
van_keys = set(re.findall(r'(?m)^ ([\w.]+):', van_loc))
sample = sorted(vanilla_cultures)[:400]
van_named = sum(1 for c in sample if c in van_keys)
van_adj = sum(1 for c in sample if c + '_adj' in van_keys)
print(f'\nvanilla localizes {van_named}/{len(sample)} sampled cultures by name, {van_adj} with _adj')
named = sum(1 for c in ours if c in our_keys)
adjective = sum(1 for c in ours if c + '_adj' in our_keys)
print(f'ours: {named}/{len(ours)} named, {adjective} with _adj')
missing = [c for c in ours if c not in our_keys]
if missing:
    print('  unlocalized:', missing[:8])

# Are they actually used, and does everything used actually exist?
used = collections.Counter()
start = os.path.join(MOD, r'main_menu\setup\start')
for name in ('06_pops.txt', '10_countries.txt', '05_characters.txt'):
    text = read(os.path.join(start, name))
    for match in re.finditer(r'\b(?:culture|culture_definition|primary_culture)\s*=\s*(\w+)', text):
        used[match.group(1)] += 1
    for key in ('accepted_cultures', 'tolerated_cultures'):
        for match in re.finditer(key + r'\s*=\s*\{([^}]*)\}', text):
            for culture in match.group(1).split():
                used[culture] += 1
defs = set(vanilla_cultures) | set(ours)
unknown = {c: n for c, n in used.items() if c not in defs}
print(f'\n{len(used)} distinct cultures referenced by the mod, {sum(used.values())} references')
print(f'references to cultures nothing defines: {len(unknown)} {sorted(unknown)[:8]}')
generated_used = {c: used.get(c, 0) for c in ours}
unused = [c for c, n in generated_used.items() if n == 0]
print(f'generated cultures actually used: {len(ours) - len(unused)}/{len(ours)}, unused: {unused[:8]}')
busiest = sorted(generated_used.items(), key=lambda kv: -kv[1])[:5]
print('busiest generated cultures:', busiest)

# Pops carry a culture and a religion; a pop culture with no definition is the crash-adjacent case.
pops = read(os.path.join(start, '06_pops.txt'))
pop_cultures = collections.Counter(re.findall(r'culture\s*=\s*(\w+)', pops))
bad_pops = {c: n for c, n in pop_cultures.items() if c not in defs}
print(f'\npop culture references: {sum(pop_cultures.values())}, undefined: {bad_pops}')
