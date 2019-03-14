#!/usr/bin/env python3
""" Read translation strings and ensure enough memory is allocated in Tx for parsing them"""

import argparse
import os
import sys
import subprocess
import glob
import re
from functools import total_ordering



TARGETS = ["devo8", "devo10", "devo12"]
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
LOG = []

ERROR = [0]

@total_ordering
class MaxVal:
    """An integer that can be updated with a larger value"""
    def __init__(self):
        """Initialization"""
        self.val = 0

    def update(self, val):
        """Update value if larger"""
        if val > self.val:
            self.val = val

    def __eq__(self, other):
        """Equivalence"""
        return self.val == other

    def __ne__(self, other):
        """Non-equivalence"""
        return not self == other

    def __lt__(self, other):
        """less than"""
        return self.val < other

    def __format__(self, spec):
        """format"""
        return self.val.__format__(spec)

def main():
    """Main routine"""
    parser = argparse.ArgumentParser()
    parser.add_argument("-target", required=True,
                        help="Transmitter target")
    parser.add_argument("-objdir", required=True,
                        help="Directory containing obj files")
    parser.add_argument("-quiet", action="store_true",
                        help="Silence all output except errors")
    args = parser.parse_args()

    dirs = [_d for _d in sorted(glob.glob(os.path.join("filesystem", "*")))
            if "common" not in _d and (not args.target or os.path.basename(_d) == args.target)]

    max_line_length = MaxVal()
    max_bytes = MaxVal()
    max_count = MaxVal()
    for target_dir in dirs:
        _bytes, count, line_length = read_target(target_dir, args.objdir)
        max_bytes.update(_bytes)
        max_count.update(count)
        max_line_length.update(line_length)

    log("{:35}: {:5d} lines, {:5d} bytes, {:4d} bytes/line"
        .format("Total", max_count, max_bytes, max_line_length))
    allowed_bytes, allowed_count, allowed_line_length = read_language_c()
    log("{:35}: {:5d} lines, {:5d} bytes, {:4d} bytes/line"
        .format("Allocated", allowed_count, allowed_bytes, allowed_line_length))
    if (max_line_length > allowed_line_length
            or max_count > allowed_count
            or max_bytes > allowed_bytes):
        set_error()
    if not args.quiet or get_error():
        if get_error():
            print("ERROR: Not enough stringspace allocated:")
        for _l in LOG:
            print(_l)
    return not get_error()


def read_target(target_dir, objdir):
    """Read all language files for a target and calculate max usage"""
    target = os.path.basename(target_dir)
    langfiles = glob.glob(os.path.join(target_dir, "language", "*"))
    if not langfiles:
        print("WARNING: No language files found for {}.  Assuming no language support"
              .format(target))
        sys.exit(0)
    target_bytes = MaxVal()
    target_line_count = MaxVal()
    target_max_line_length = MaxVal()
    log("Directory: " + target_dir)
    for filename in sorted(langfiles):
        lines = open(filename, "r", encoding='utf-8').read().splitlines()
        lines.pop(0)
        _bytes, line_count, max_line_length = parse_v1_file(lines)
        log("{:35}: {:5d} lines, {:5d} bytes, {:4d} bytes/line"
            .format(filename, line_count, _bytes, max_line_length))
        target_bytes.update(_bytes)
        target_line_count.update(line_count)
        target_max_line_length.update(max_line_length)
    language = get_language(target)
    cmd = (os.path.join(SCRIPT_DIR, "extract_strings.py")
           + " -target " + language
           + " -objdir " + objdir)
    line_count = len(system(cmd).splitlines())
    log("{:35}: {:5d} lines".format("Source strings", line_count))
    target_line_count.update(line_count)
    log("{:35}: {:5d} lines, {:5d} bytes, {:4d} bytes/line"
        .format(target_dir, target_line_count,
                target_bytes, target_max_line_length))
    return (target_bytes, target_line_count, target_max_line_length)


def parse_v1_file(lines):
    """Parse language file"""
    fnvhash = {}
    _bytes = 0
    line_count = 0
    max_line_length = MaxVal()
    #import pdb; pdb.set_trace()
    while lines:
        line = lines.pop(0)
        if line[0] == ':':
            translation = lines.pop(0)
            hashval = fnv_16(line[1:])
            if hashval in fnvhash:
                print("Found hash collision between:\n    {}\n    {}"
                      .format(fnvhash[hashval], line[1:]))
                set_error()
            else:
                fnvhash[hashval] = line[1:]
            length = len(translation.encode('utf-8')) + 1    # Include the NULL terminator
            _bytes += length
            line_count += 1
            max_line_length.update(length)
    return (_bytes, line_count, max_line_length)


def get_language(target):
    """Get language values from Makefile"""
    path = glob.glob(os.path.join("target", "tx", "*", target, "Makefile.inc"))[0]
    with open(path, "r") as _fh:
        for line in _fh:
            _m = re.search(r'^\s*LANGUAGE\s+:?=\s*(.*\S)', line)
            if _m:
                return _m.group(1)
    print("ERROR: Can't extract LANGUAGE variable from path")
    sys.exit(1)


def read_language_c():
    """Read language.c to find target limits"""
    allowed_line_length = 0
    allowed_count = 0
    allowed_bytes = 0
    try:
        lines = open("config/language.c", "r").readlines()
        for line in lines:
            _m = re.search(r'char strings\[\s*(\d+)\s*\]', line)
            if _m:
                allowed_bytes = int(_m.group(1))
            _m = re.search(r'MAX_STRINGS\s+(\d+)', line)
            if _m:
                allowed_count = int(_m.group(1))
            _m = re.search(r'MAX_LINE\s+(\d+)', line)
            if _m:
                allowed_line_length = int(_m.group(1))
            if allowed_line_length and allowed_count and allowed_bytes:
                break
    except OSError:
        log("ERROR: Couldn't read config/language.c")
        set_error()
    return (allowed_bytes, allowed_count, allowed_line_length)


def set_error():
    """Set global error flag"""
    ERROR[0] = 1


def get_error():
    """Return error status"""
    return ERROR[0]


def fnv_16(data, init_value=0x811c9dc5):
    """Generate a FNV32 hash, and fold it into a 16bit value"""
    data = data.encode('utf-8')
    fnv_32_prime = 0x01000193
    hval = init_value

    for _ch in data:
        hval = (hval * fnv_32_prime) & 0xffffffff
        hval = (hval ^ _ch) & 0xffffffff
    return (hval >> 16) ^ (hval & 0xffff)


def log(msg):
    """Add log message"""
    LOG.append(msg)

def system(cmd):
    """Wrapper to easily call system functions"""
    #log("Running: " + "".join(cmd))
    if isinstance(cmd, list):
        return subprocess.check_output(cmd).decode('utf-8').rstrip()
    return subprocess.check_output(cmd, shell=True).decode('utf-8').rstrip()

sys.exit(0 if main() else 1)
