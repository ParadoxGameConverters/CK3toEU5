"""
Buckets an EU5 error log into recurring shapes so a mod's remaining noise reads at a glance.

Point it at the log folder; it reads error.log and its rotated siblings.
"""
import os
import re
import sys
import collections

LOGS = sys.argv[1] if len(sys.argv) > 1 else \
    r'C:\Users\kyleo\OneDrive\Documents\Paradox Interactive\Europa Universalis V\logs'

SHAPES = [
    (r"does not exist, nor has cores as a revolter at start", 'country does not exist at start'),
    (r"[Ss]etting a law .* advance", 'law without the advance'),
    (r"have the same name", 'duplicate country name'),
    (r"[Pp]ort filter used on a location", 'port filter on an inland location'),
    (r"already owned by", 'location owned twice'),
    (r"Duplicate key", 'duplicate key in a database'),
    (r"[Ii]nvalid building", 'invalid building for its location'),
    (r"no pops", 'culture with no pops'),
    (r"[Mm]issing localization|not found in any localization", 'missing localization'),
    (r"privilege", 'invalid estate privilege'),
    (r"[Pp]olicy", 'invalid policy'),
]

lines = 0
buckets = collections.Counter()
other = collections.Counter()
for name in sorted(os.listdir(LOGS)):
    if not re.match(r'error(\.\d+)?\.log$', name):
        continue
    with open(os.path.join(LOGS, name), encoding='utf-8', errors='replace') as handle:
        for line in handle:
            lines += 1
            for pattern, label in SHAPES:
                if re.search(pattern, line):
                    buckets[label] += 1
                    break
            else:
                trimmed = re.sub(r'[\'"][^\'"]*[\'"]', 'X', line.strip())
                trimmed = re.sub(r'\d+', 'N', trimmed)
                other[trimmed[:110]] += 1

print(f'{lines} lines across the error logs')
for label, count in buckets.most_common():
    print(f'{count:>7}  {label}')
print('\nunbucketed shapes:')
for shape, count in other.most_common(15):
    print(f'{count:>7}  {shape}')
