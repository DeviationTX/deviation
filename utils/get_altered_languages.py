#!/usr/bin/env python

import subprocess
import re
import sys

cmd = ['git', 'status']

msg = sys.argv[1]

res = subprocess.Popen(cmd, stdout=subprocess.PIPE)
lines = [line.rstrip() for line in res.stdout]

languages = {}

pat = re.compile(r'locale/deviation\.(.+)\.po$')
for line in lines:
    match = pat.search(line)
    if match:
        languages[match.group(1)] = 1
if languages:
    msg += " (Languages: " + " ".join(sorted(languages.keys())) + ")"
print msg
