#!/usr/bin/env python3

# Extract translatable strings out of object files and
# compare/convert to po translation files

import argparse
import os
import sys
import re
import subprocess
import logging
import glob

PO_LANGUAGE_STRING = "->Translated Language Name<-"
TARGETS = ["devo8", "devo10", "devo12"]
CROSS = os.environ.get("CROSS", "")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-update", action="store_true",
                        help="Update existing po files with modified string lists")
    parser.add_argument("-language",
                        help="Language or pattern of language files to update")
    parser.add_argument("-fs",
                        help="Directory of output language files")
    parser.add_argument("-targets", nargs='+', choices=TARGETS,
                        help="Specify targets to generate")
    parser.add_argument("-count", action="store_true",
                        help="Generate count of unique srings")
    parser.add_argument("-po", action="store_true",
                        help="Read po files rather than devo-formatted files")
    parser.add_argument("-objdir",
                        help="Directory containing .o files to extract strings from")
    args = parser.parse_args()

    if args.fs and not args.targets:
        logging.error("Must specify both -targets with -fs")
        return False

    uniq = get_strings(args.objdir)

    if args.count:
        print("{}".format(len(uniq.keys())))
        return True

    if not args.update:
        if args.po:
            print_po_strings(uniq)
        else:
            print_strings(uniq)
        return True
    files = (["fs/language/locale/deviation.{}.po".format(args.language)] if args.language
             else glob.glob("fs/language/locale/*.po"))
    for filename in files:
        (ext, language, translation) = parse_po_file(filename, uniq)
        if not write_lang_file("{}/lang.{}".format(args.fs, ext), args.targets, language, translation):
            return False
    return True

def print_po_strings(uniq):
    print('msgid "{}"\nmsgstr ""\n'.format(PO_LANGUAGE_STRING));
    for string in uniq['__ORDER__']:
        # Match same syntax used by getopt
        print('\n'.join(uniq[string]))  # Comment
        # we need to split on '\\n' but preserver it as part of the previous line
        # python doesn't offer 'split' based on zero-length look-behind
        msg = [x for x in re.sub(r'\\n', r'\\n\n', string).split('\n') if x]
        if len(msg) > 1:
            print('msgid ""\n"{}"'.format('"\n"'.join(msg)))
        else:
            print('msgid "{}"'.format(msg[0]))
        print('msgstr ""')
        print('')

def print_strings(uniq):
    for string in uniq['__ORDER__']:
        print(":{}".format(string))

def get_strings(path):
    strings = extract_all_strings()
    if path:
        strings = extract_target_strings(path, strings)
    # append template names
    for line in system("head -n 1 fs/common/template/*.ini").split('\n'):
        _m = re.search(r'template=(.*?)\s*$', line)
        if _m:
            if _m.group(1) not in strings:
                strings['__ORDER__'].append(_m.group(1))
            strings[_m.group(1)] = ["#: Model template"];
    return strings

def extract_all_strings():
    lines = system("/usr/bin/find . -name '*.[hc]' | grep -v libopencm3 | sort | xargs xgettext -o - --omit-header -k --keyword=_tr --keyword=_tr_noop --no-wrap").split('\n')
    idx = 0
    strings = { "__ORDER__": [] }
    while lines:
        msgid, msgstr, comment = parse_gettext(lines)
        if not msgid:
            continue
        if msgid not in strings:
            strings['__ORDER__'].append(msgid)
        strings[msgid] = comment
    return strings

def extract_target_strings(path, valid_strings):
    strings = {}
    files = glob.glob(os.path.join(path, "*.o"))

    for filename in files:
        # Parse all strings from the object files and add to the allstr hash
        objdump = system(CROSS + "objdump -s " + filename).split('\n')
        _str = ""
        state = 0
        for line in objdump:
            if re.search(r'section (?:(?:(\.rel)?\.ro?data)|__cstring)', line):
                _str = ""
                state = 1
                continue
            if re.search(r'Contents', line) or re.search(r'\.ro?data', line):
                _str = ""
                state = 0
                continue
            if not state:
                continue
            line = re.sub(r'^\s+\S+\s', r'', line, 1)
            line = re.sub(r'\s\s.*', r'', line, 1)
            line = re.sub(r'\s', '', line)
            for _ch in re.findall(r'\S\S', line):
                if _ch == "00":
                    # NULL termination
                    if _str:
                        _str = _str.replace('\n', '\\n')
                        if _str in valid_strings:
                            strings[_str] = valid_strings[_str]
                        _str = ""
                    continue
                _str += chr(int(_ch, 16))
    strings['__ORDER__'] = [_x for _x in valid_strings['__ORDER__'] if _x in strings]
    return strings

def fnv_16(data, init_value=0x811c9dc5):
    data = data.encode('utf-8')
    fnv_32_prime = 0x01000193
    hval = init_value

    for _ch in data:
        hval = (hval * fnv_32_prime) & 0xffffffff
        hval = (hval ^ _ch) & 0xffffffff
    return (hval >> 16) ^ (hval & 0xffff);


def write_lang_file(outf, targets, language, translation):
    strings = {}
    hashvalues = {}

    for target in ["DEFAULT"] + targets:
        # Hierarchically try to find best string
        if target in translation:
            strings.update(translation[target])
    for string in sorted(strings.keys()):
        value = strings[string]
        if string == value:
            continue
        hval = fnv_16(string)
        if hval in hashvalues:
            logging.error("Conflict hash detected:\n%s\n%s",
                          hashvalues[hval], value)
            return False
        hashvalues[hval] = value
    try:
        with open(outf, "w") as _fh:
            _fh.write(language)
            for key in sorted(strings.keys()):
                value = strings[key]
                if key == value:
                    continue
                _fh.write(":{}\n{}\n".format(key, value))
    except OSError:
        logging.error("Can't write " + outf)
        return False
    return True

def parse_gettext(lines):
    """Parse next gettext element for string list"""

    msgid = None
    comment = []
    while True:
        line = lines.pop(0)
        if re.search(r'^#[:,]', line):
            comment.append(line)
            continue
        _m = re.search(r'^\s*(msgid|msgstr)\s+"(.*)"\s*$', line)
        if _m:
            _type = _m.group(1)
            _str = _m.group(2)
            while lines:
                _m = re.search(r'^\s*"(.*)"\s*$', lines[0])
                if not _m:
                    break
                _str += _m.group(1)
                lines.pop(0)
            if _type == "msgid":
                if msgid:
                    logging.error("msgid '%s' missing msgstr.  Ignoring", msgid)
                msgid = _str
            elif _type == "msgstr":
                if msgid is None:
                    logging.error("no msgid for msgstr '%s'.  Ignoring", msgstr)
                    continue
                return(msgid, _str, comment or "#")
    return (None, None, None)

def parse_po_file(filename, uniq):
    """Parse .po file into dictionary"""
    strings = {'DEFAULT': {}}
    language = "Unknown"
    ext = None
    re_target = "|".join(TARGETS)

    lines = open(filename, "r", encoding='utf-8').read().splitlines()
    while lines:
        msgid, msgstr, comment = parse_gettext(lines)
        if msgstr:
            msgstr = re.sub(r'\\([^n])', r'\1', msgstr)  # Fix escaped characters
        if not msgstr:
            continue
        if msgid == "":
            _m = re.search(r'(?:\b|\\n)Language:\s+(\S+?)(?:\b|\\n)', msgstr)
            if _m:
                ext = _m.group(1).lower()
                if len(ext) > 3:
                    ext = re.sub(r'^.*_', r'', ext)
                    ext = ext[-3:]
            continue
        if msgid == PO_LANGUAGE_STRING:
            # Add UTF-8 BOM
            language = '\ufeff' + msgstr + "\n";
            continue
        if msgid not in uniq:
            continue
  
        values = re.split(r'\\n(?=(?:{})?:)'.format(re_target), msgstr)
        for value in values:
            _m = re.search(r'(?:({}):)?(.*)'.format(re_target), value)
            target = _m.group(1)
            target_str = _m.group(2)
            if target:
                if target not in strings:
                    strings[target] = {}
                strings[target][msgid] = target_str
            if msgid not in strings['DEFAULT']:
                strings['DEFAULT'][msgid] = target_str
    return (ext, language, strings)

def system(cmd):
    logging.debug("Running: " + " ".join(cmd))
    if isinstance(cmd, list):
        return subprocess.check_output(cmd).decode('utf-8').rstrip()
    else:
        return subprocess.check_output(cmd, shell=True).decode('utf-8').rstrip()

sys.exit(0 if main() else 1)
