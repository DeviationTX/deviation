#!/usr/bin/env python3

import subprocess
import re
import sys

cmd = ['git', 'status']

msg = sys.argv[1]

res = subprocess.Popen(cmd, stdout=subprocess.PIPE)
lines = [line.decode('utf-8').rstrip() for line in res.stdout]

languages = {}

pat = re.compile(r'locale/deviation\.(.+)\.po$')
english = False
for line in lines:
    if re.search(r'deviation.po$', line):
        english = True
    match = pat.search(line)
    if match:
        languages[match.group(1)] = 1
lang = sorted(languages.keys())
if english:
    lang.insert(0, "en")
if lang:
    msg += " (Languages: " + " ".join(lang) + ")"
print(msg)
