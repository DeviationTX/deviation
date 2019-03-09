#!/usr/bin/env python3

import os
import subprocess
import sys

cmd = ['git', 'diff', '--relative']
if len(sys.argv) > 1:
    cmd += sys.argv[1:]

res = subprocess.Popen(cmd, stdout=subprocess.PIPE)
lines = [line.decode('utf-8').rstrip() for line in res.stdout]
lines.append("diff") # Crude way to ensure final file is handled

filename = None
changes = 0
for line in lines:
    if line.startswith("diff"):
        if filename:
            if not changes:
                subprocess.call(['git', 'checkout' , '--', filename])
            else:
                print("Found changes in: " + filename)
            filename = None
        changes = 0
        filename = os.path.sep.join(line.split(" ")[-1].split(os.path.sep)[1:]) # FIXME: This is not x-platform clean
    if line.startswith(('-', '+')) and not line.startswith(('---', '+++')):
        if not any(line[1:].startswith(x) for x in ['"POT-Creation-Date: ', '"Last-Translator: ', '#']):
            changes += 1


