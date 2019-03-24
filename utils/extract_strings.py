#!/usr/bin/env python3
"""Extract translatable strings out of object files and
   compare/convert to po translation files or Deviation language files
"""

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
os.environ['LANG'] = 'en_US.UTF-8'  # We need English to be able to regex parse cmd output

def main():
    """Main routine"""
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
        if not write_lang_file("{}/lang.{}".format(args.fs, ext),
                               args.targets, language, translation):
            return False
    return True


def print_po_strings(uniq):
    """Generate po file containing all translatable strings"""
    print('msgid "{}"\nmsgstr ""\n'.format(PO_LANGUAGE_STRING))
    for string in uniq['__ORDER__']:
        # Match same syntax used by getopt
        print('\n'.join(uniq[string]))  # Comment
        # we need to split on '\\n' but preserver it as part of the previous line
        # python doesn't offer 'split' based on zero-length look-behind
        msg = [x for x in string.replace('\\n', '\\n\n').split('\n') if x]
        if len(msg) > 1:
            print('msgid ""\n"{}"'.format('"\n"'.join(msg)))
        else:
            print('msgid "{}"'.format(msg[0]))
        print('msgstr ""')
        print('')


def print_strings(uniq):
    """Generate deviation lang file containing all translatable strings"""
    for string in sorted(uniq['__ORDER__']):
        print(":{}".format(string))


def get_strings(path):
    """Get the full list of translatable strings, potentially filtered by target"""
    strings = extract_all_strings()
    if path:
        strings = extract_target_strings(path, strings)
    # append template names
    _re = re.compile(r'template=(.*?)\s*$')
    for line in system("head -n 1 fs/common/template/*.ini").split('\n'):
        _m = _re.search(line)
        if _m:
            if _m.group(1) not in strings:
                strings['__ORDER__'].append(_m.group(1))
            strings[_m.group(1)] = ["#: Model template"]
    return strings


def extract_all_strings():
    """Extaract full list of translatable strings from source-code"""
    strings = {"__ORDER__": []}
    cmd = ("find . -name '*.[hc]' | grep -v libopencm3 | sort "
           "| xargs xgettext -o - --omit-header -k --keyword=_tr "
           "--keyword=_tr_noop --no-wrap")
    _p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    while True:
        msgid, _msgstr, comment = parse_gettext(_p.stdout)
        if msgid is None:
            break
        if not msgid:
            continue
        if msgid not in strings:
            strings['__ORDER__'].append(msgid)
        strings[msgid] = comment
    return strings


def extract_target_strings(path, valid_strings):
    """Extract all strings from target object files"""
    strings = {}

    cmd = CROSS + "objdump -s " + os.path.join(path, "*.o")
    _p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    _str = ""
    state = 0
    for line in _p.stdout:
        line = line.decode('utf-8')
        if not line.startswith(' '):
            state = 0
            _str = ""
            if line.startswith("Contents") and any(x in line for x in [
                    "section .rel.rdata", "section .rel.rodata",
                    "section .rdata", "section .rodata", "section __cstring"]):
                state = 1
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
    """Generate a FNV32 hash, and fold it into a 16bit value"""
    data = data.encode('utf-8')
    fnv_32_prime = 0x01000193
    hval = init_value

    for _ch in data:
        hval = (hval * fnv_32_prime) & 0xffffffff
        hval = (hval ^ _ch) & 0xffffffff
    return (hval >> 16) ^ (hval & 0xffff)


def write_lang_file(outf, targets, language, translation):
    """Write Deviation lang file for selected language"""
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
        with open(outf, "wb") as _fh:
            _fh.write(language.encode('utf-8'))
            for key in sorted(strings.keys()):
                value = strings[key]
                if key == value:
                    continue
                _fh.write(":{}\n{}\n".format(key, value).encode('utf-8'))
    except OSError:
        logging.error("Can't write %s", outf)
        return False
    return True


def parse_gettext(_fh):
    """Parse next gettext element for string list"""

    msgid = None
    comment = []
    in_multiline = False
    re_msg = re.compile(r'^\s*(msgid|msgstr)\s+"(.*)"\s*$')
    re_quoted = re.compile(r'^\s*"(.*)"\s*$')
    _str = ""
    _type = ""
    # import pdb; pdb.set_trace()
    while True:
        line = _fh.readline().decode('utf-8')
        if in_multiline:
            _m = re_quoted.search(line)
            if _m:
                _str += _m.group(1)
                continue
            in_multiline = False
            if _type == "msgid":
                if msgid:
                    logging.error("msgid '%s' missing msgstr.  Ignoring", msgid)
                msgid = _str
            elif _type == "msgstr":
                if msgid is not None:
                    return(msgid, _str, comment or "#")
                logging.error("no msgid for msgstr '%s'.  Ignoring", _str)
        if not line:
            break
        if line.startswith('#:') or line.startswith('#,'):
            comment.append(line.rstrip())
            continue
        _m = re_msg.search(line)
        if _m:
            _type = _m.group(1)
            _str = _m.group(2)
            in_multiline = True
    return (None, None, None)


def parse_po_file(filename, uniq):    # pylint: disable=too-many-branches
    """Parse .po file into dictionary"""
    strings = {'DEFAULT': {}}
    language = "Unknown"
    ext = None
    re_target = "|".join(TARGETS)

    _fh = open(filename, "rb")
    while True:
        msgid, msgstr, _comment = parse_gettext(_fh)
        if msgid is None:
            break
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
            language = '\ufeff' + msgstr + "\n"
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
    """Wrapper to easily call system functions"""
    logging.debug("Running: %s", " ".join(cmd))
    if isinstance(cmd, list):
        return subprocess.check_output(cmd).decode('utf-8').rstrip()
    return subprocess.check_output(cmd, shell=True).decode('utf-8').rstrip()

sys.exit(0 if main() else 1)
