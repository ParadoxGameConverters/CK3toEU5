import re, os, glob, collections

root = r"C:\Users\kyleo\OneDrive\Desktop\Converters\CK3toEU5\build\Release-Windows\CK3toEU5\output\Malik_al_Muazzam_Danyal_of_the_Yamamoto_Empire_1407_05_20"
countries = open(os.path.join(root, "main_menu", "setup", "start", "10_countries.txt"), encoding="utf-8-sig").read()

# tag -> (include, tech level)
info = {}
tag = None
for line in countries.splitlines():
    m = re.match(r"\s*([A-Z][A-Z0-9]{2}) = \{", line)
    if m:
        tag = m.group(1)
        info[tag] = {}
        continue
    if tag is None:
        continue
    m = re.search(r'include = "(\w+)"', line)
    if m:
        info[tag]["include"] = m.group(1)
    m = re.search(r"starting_technology_level = (\d+)", line)
    if m:
        info[tag]["tech"] = int(m.group(1))

logs = glob.glob(os.path.join(os.path.expanduser("~"), "OneDrive", "Documents", "Paradox Interactive",
                              "Europa Universalis V", "logs", "error*.log"))
bad = collections.defaultdict(set)
for f in logs:
    for line in open(f, encoding="utf-8", errors="ignore"):
        m = re.search(r"Setting a law - (\w+) - for '(\w+)", line)
        if m:
            bad[m.group(2)].add(m.group(1))

techs_bad = collections.Counter()
techs_ok = collections.Counter()
for t, d in info.items():
    if "tech" not in d:
        continue
    (techs_bad if t in bad else techs_ok)[d["tech"]] += 1

print("tech level of countries WITH law errors:", dict(sorted(techs_bad.items())))
print("tech level of countries without:      ", dict(sorted(techs_ok.items())))
print()
for t in sorted(bad)[:5]:
    print(t, info.get(t), sorted(bad[t]))
