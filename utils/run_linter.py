#!/usr/bin/env python
"""Run cpplint linter on C code

Usage:
    run_linter.py [--diff] [paths]

When used with --diff, will only show lint errors for uncommitted changes
or, if run with Travis-CI, lint-errors on lines changed by a Pull Request
"""

import os
import re
import sys
import json
import urllib2
import zlib
import argparse
import subprocess
from collections import namedtuple


LINT_RULES = (
    '-whitespace/line_length',
    '-legal',
    '-build/header_guard',
    '-build/include',
    '-build/include_subdir',
    '-readability/casting')

EXCLUDE_PATHS = ('libopencm3/', 'FatFs/')

def main():
    with open(os.devnull, 'w') as devnull:
        if subprocess.call(["which", "cpplint"], stdout=devnull):
            print("Please install cpplint via 'pip install cpplint' or equivalent")
            sys.exit(1)

    parser = argparse.ArgumentParser()
    parser.add_argument("path", nargs='*', help="Paths to lint")
    parser.add_argument("--diff", action="store_true",
                        help="Only lint uncommitted lines (or those from Travis-CI PR)")
    parser.add_argument("--debug", action="store_true",
                        help="Add debug output")
    args = parser.parse_args()

    changed = {}
    if args.diff:
        changed = get_changed_lines(args.debug)

    paths = filter_paths(args.path, changed)
    run_lint(paths, changed, args.debug)

def get_changed_lines(debug):
    changed = {}
    cmd = ["git", "diff", "--relative", "--name-only", "--diff-filter", "AM"]
    sha1 = "000000000";
    if 'TRAVIS_BRANCH' in os.environ:
        cmd += "HEAD..{}".format(os.environ['TRAVIS_BRANCH'])
        sha1 = subprocess.check_output(
            ["git", "rev-parse", "--short=9", os.environ['TRAVIS_BRANCH']]).rstrip()
    files = subprocess.check_output(cmd).split("\n")
    for _file in [_f.rstrip() for _f in files]:
        if not _file:
            continue
        changed[_file] = {}
        _p = subprocess.Popen(["git", "blame", _file], stdout=subprocess.PIPE)
        for line in _p.stdout:
            match = re.search("^{}\s[^\(]*\([^\)]*\s(\d+)\)".format(sha1), line)
            if not match:
                continue
            changed[_file][match.group(1)] = 1
        if debug:
            print("{}: {}".format(_file, sorted(changed[_file].keys())))
    return changed

def filter_paths(paths, changed):
    ret = []
    if not changed:
        if not paths:
            return ["."]
        return paths
    if not paths:
        return sorted(changed.keys())
    cmd = "find {} -type f".format(" ".join(paths))
    if EXCLUDE_PATHS:
        cmd += " | grep -v -E '({})'".format(" ".join(EXCLUDE_PATHS))
    _p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    for line in _p.stdout:
        line = line.rstrip()
        if line.startswith("./"):
            line = line[2:]
        if line in changed:
            ret += line
    return ret

def run_lint(paths, changed, debug):
    violations = {}
    errors = {}
    cmd = 'find {} -name "*.[ch]"'.format(" ".join(paths))
    if EXCLUDE_PATHS:
        cmd += " | grep -v -E '({})'".format(" ".join(EXCLUDE_PATHS))
    cmd += " | xargs cpplint --extensions=c,h --filter={} 2>&1".format(",".join(LINT_RULES))
    if debug:
        print cmd
    _p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    for line in _p.stdout:
        line = line.rstrip()
        match = re.search("(\S+):(\d+):\s.*\[(\S+)\]\s\[\d\]$", line)
        if match:
            filename = match.group(1)
            linenum = match.group(2)
            violation = match.group(3)
            if not changed or linenum not in changed[filename]:
                continue
            print line
            if filename not in errors:
                errors[filename] = 0
            errors[filename] += 1
            if violation not in violations:
                violations[violation] = 0
            violations[violation] += 1
    print "\nSummary\n-------";
    for err in sorted(violations.keys()):
        print "{:30s}: {}".format(err, violations[err])
main()
