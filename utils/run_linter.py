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
import time
import logging
from collections import namedtuple


LINT_RULES = (
    '-whitespace/line_length',
    '-legal',
    '-build/header_guard',
    '-build/include',
    '-build/include_subdir',
    '-readability/casting')

EXCLUDE_PATHS = ('libopencm3/', 'FatFs/')

UNMATCHED_LINT_ERROR = "Unmatched Lint Error(s):\n"
LINT_ERROR = "Lint Error:\n"

GITHUB_TOKEN = os.environ.get('GITHUB_TOKEN')
TRAVIS_PULL_REQUEST = os.environ.get('TRAVIS_PULL_REQUEST')
TRAVIS_PULL_REQUEST_SHA = os.environ.get('TRAVIS_PULL_REQUEST_SHA')
TRAVIS_BRANCH = os.environ.get('TRAVIS_BRANCH')
TRAVIS_REPO_SLUG = os.environ.get('TRAVIS_REPO_SLUG')

if TRAVIS_PULL_REQUEST == 'false':
    TRAVIS_PULL_REQUEST = False


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
    parser.add_argument("--skip-github", action="store_true",
                        help="Don't update GitHub")
    args = parser.parse_args()
    if args.debug:
        logging.basicConfig(level=logging.DEBUG)

    pwd = os.getcwd()
    os.chdir(system(["git", "rev-parse", "--show-toplevel"]).rstrip())

    logging.debug("TRAVIS_PULL_REQUEST:     %s",  TRAVIS_PULL_REQUEST)
    logging.debug("TRAVIS_PULL_REQUEST_SHA: %s",  TRAVIS_PULL_REQUEST_SHA)
    logging.debug("TRAVIS_BRANCH:           %s", TRAVIS_BRANCH)
    logging.debug("TRAVIS_REPO_SLUG:        %s", TRAVIS_REPO_SLUG)

    changed = {}
    if args.diff:
        changed = get_changed_lines()

    paths = filter_paths(args.path, changed, pwd)
    violations = run_lint(paths, changed)
    if GITHUB_TOKEN and TRAVIS_PULL_REQUEST and not args.skip_github:
        update_github_status(violations)

def get_changed_lines():
    changed = {}
    master = TRAVIS_BRANCH or "master"
    base = system(["git", "merge-base", "HEAD", master]).rstrip()
    cmd = ["git", "diff", "--name-only", "--diff-filter", "AM", base]
    sha1s = ["000000000"];
    # Find all sha's on this branch
    sha1s += [commit[:9] for commit in system(["git", "rev-list", base + "..HEAD"]).rstrip().split('\n')]
    logging.debug("Mathing SHA1s: %s", sha1s)
    files = system(cmd).rstrip().split("\n")
    for _file in files:
        changed[_file] = {}
        _p = subprocess.Popen(["git", "blame", _file], stdout=subprocess.PIPE)
        for line in _p.stdout:
            logging.debug(line)
            sha = line[:9]
            if sha not in sha1s:
                continue
            match = re.search("^[^\(]*\([^\)]*\s(\d+)\)", line)
            if not match:
                continue
            changed[_file][int(match.group(1))] = 1
        logging.debug("%s: %s", _file, sorted(changed[_file].keys()))
    return changed

def filter_paths(paths, changed, pwd):
    ret = []
    root = os.getcwd()  # We already cd'd to GIT_ROOT
    common = os.path.commonprefix([root, pwd])
    relpath = os.path.relpath(pwd, common)
    paths = [_p if _p.startswith(".") else os.path.join(relpath, _p) for _p in paths]
    if not changed:
        if not paths:
            return relpath
        return paths
    if not paths:
        return sorted(changed.keys())
    cmd = "find {} -type f".format(" ".join(paths))
    if EXCLUDE_PATHS:
        cmd += " | grep -v -E '({})'".format(" ".join(EXCLUDE_PATHS))
    logging.debug("Running: " + " ".join(cmd))
    _p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    for line in _p.stdout:
        line = line.rstrip()
        if line.startswith("./"):
            line = line[2:]
        if line in changed:
            ret += line
    return ret

def run_lint(paths, changed):
    violations = {}
    count = {}
    errors = {}
    
    cmd = 'find {} -name "*.[ch]"'.format(" ".join(paths))
    if EXCLUDE_PATHS:
        cmd += " | grep -v -E '({})'".format(" ".join(EXCLUDE_PATHS))
    cmd += " | xargs cpplint --extensions=c,h --filter={} 2>&1".format(",".join(LINT_RULES))
    logging.debug("Running: " + cmd)
    _p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    for line in _p.stdout:
        line = line.rstrip()
        match = re.search("(\S+):(\d+):\s.*\[(\S+)\]\s\[\d\]$", line)
        if match:
            filename = match.group(1)
            linenum = int(match.group(2))
            violation = match.group(3)
            if not changed or linenum not in changed[filename]:
                continue
            print line
            if filename not in errors:
                errors[filename] = 0
            errors[filename] += 1
            if violation not in count:
                count[violation] = 0
            count[violation] += 1
            if filename not in violations:
                violations[filename] = {}
            if linenum not in violations[filename]:
                violations[filename][linenum] = []
            violations[filename][linenum].append(line)

    print "\nSummary\n-------";
    for err in sorted(count.keys()):
        print "{:30s}: {}".format(err, count[err])
    return violations

def update_github_status(violations):
    clean_old_comments()

    url = 'https://api.github.com/repos/{}/pulls/{}'.format(TRAVIS_REPO_SLUG, TRAVIS_PULL_REQUEST)
    # _pr = json.loads(get_url(url))
    #status_url = _pr['statuses_url']
    diff = get_url(url, {"Accept": "application/vnd.github.v3.diff"}).split('\n')
    unassigned = []
    count = 0
    for _file in violations:
        count += len(violations[_file])
        groups = group_lines(violations[_file])
        for group in groups:
            offset = compute_offset(diff, _file, group)
            if offset is not None:
                post_pull_comment(_file, offset, format_msg(LINT_ERROR, group["msgs"], True))
            else:
                unassigned += group["msgs"]
    if unassigned:
        post_issue_comment(format_msg(UNMATCHED_LINT_ERROR, unassigned))

    if violations:
        post_status("failure", "linter", "Linter had {} errors".format(count))
    else:
        post_status("success", "linter", "Linter had no errors")

def format_msg(header, msgs, skipfile=False):
    outmsg = header + "```\n"
    for msg in msgs:
       _file, linenum, _str= msg.split(":", 2)
       _str = re.sub('\[[^\[]+\]\s+\[\d+\]\s*$', '', _str)
       if skipfile:
           outmsg += linenum + ":" + _str + "\n"
       else:
           outmsg += _file + ':' + linenum + ":" + _str + '\n'
    outmsg += "```"
    return outmsg

def clean_old_comments():
    url = 'https://api.github.com/repos/{}/pulls/{}/comments'.format(TRAVIS_REPO_SLUG, TRAVIS_PULL_REQUEST)
    comments = json.loads(get_url(url))
    for comment in comments:
        if comment['body'].startswith(LINT_ERROR):
            delete_comment("pulls", comment['id'])
    url = 'https://api.github.com/repos/{}/issues/{}/comments'.format(TRAVIS_REPO_SLUG, TRAVIS_PULL_REQUEST)
    comments = json.loads(get_url(url))
    for comment in comments:
        if comment['body'].startswith(UNMATCHED_LINT_ERROR):
            delete_comment("issues", comment['id'])
    
def group_lines(line_err):
    """Try to group into groups of 5 lines for github"""
    linenums = sorted(line_err, key=int)
    groups = []
    group = {}
    startnum = -999
    for num in linenums:
        if num - startnum > 5:
            startnum = num
            group = {"start": num, "end": num, "msgs": []}
            groups.append(group)
        group["end"] = num
        group["msgs"] += line_err[num]
    return groups

def compute_offset(diff, filename, group):
    filename = filename
    in_file = False
    seen_at = False
    pos = 0
    offset = None
    for line in diff:
        pos += 1
        if line.startswith("diff "):
            if offset is not None:
                return offset
            in_file = line.endswith(" b/" + filename)
            seen_at = False
            continue
        if in_file:
            if any(line.startswith(pat) for pat in ["index", "---", "+++"]):
                continue
            if line.startswith("@@"):
                if offset is not None:
                    return offset
                if not seen_at:
                    seen_at = True
                    pos = 0
                match = re.search(" \+(\d+),(\d+) @@", line)
                if match:
                    file_pos = int(match.group(1))
                continue
            if line.startswith("-"):
                continue
            if file_pos >= group['start'] and file_pos <= group['end']:
                offset = pos
            elif offset is not None:
                return offset
            file_pos += 1
    return None

def raise_for_status(url, response):
    """Raise exception on URL error"""
    if response.getcode() < 200 or response.getcode() >= 300:
        sys.stderr.write(response.read())
        sys.stderr.write('\n')
        raise Exception('Request for %s failed: %s' % (url, response.getcode()))

def get_url(url, headers=None):
    logging.debug("GET: " + url)
    if not headers:
        headers = {}
    headers['Authorization'] = 'token ' + get_token()
    request = urllib2.Request(url, None, headers)
    res = urllib2.urlopen(request)
    raise_for_status(url, res)
    return res.read()

def get_token():
    """Return github token"""
    token = zlib.decompress(GITHUB_TOKEN.decode("hex"))
    length = len(token)
    return ''.join(chr(ord(a) ^ ord(b))
                   for a, b in zip(token, (TRAVIS_REPO_SLUG * length)[:length]))


def post_status(state, context, description):
    """Send status update to GitHub"""
    url = 'https://api.github.com/repos/{}/pulls/{}'.format(TRAVIS_REPO_SLUG, TRAVIS_PULL_REQUEST)
    _pr = json.loads(get_url(url))
    url = _pr['statuses_url']
    data = {
        'state': state,
        'context': context,
        'description': description
    }
    headers = {'Authorization': 'token ' + get_token()}

    logging.debug("POST: " + url)
    logging.debug("Data: " + json.dumps(data))
    request = urllib2.Request(url, json.dumps(data), headers)
    res = urllib2.urlopen(request)
    raise_for_status(url, res)


def post_pull_comment(filename, offset, message):
    url = 'https://api.github.com/repos/{}/pulls/{}/comments'.format(TRAVIS_REPO_SLUG, TRAVIS_PULL_REQUEST)
    data = {
        'commit_id': TRAVIS_PULL_REQUEST_SHA,
        'path': filename,
        'position': offset,
        'body': message,
        }
    headers = {'Authorization': 'token ' + get_token()}
    logging.debug("POST: " + url)
    logging.debug("Data: " + json.dumps(data))

    try:
        request = urllib2.Request(url, json.dumps(data), headers)
        res = urllib2.urlopen(request)
        raise_for_status(url, res)
    except urllib2.HTTPError as e:
        print e
        print e.read()
        sys.exit(1)
    time.sleep(1.0)


def post_issue_comment(message):
    url = 'https://api.github.com/repos/{}/issues/{}/comments'.format(TRAVIS_REPO_SLUG, TRAVIS_PULL_REQUEST)
    data = {
        'body': message,
        }
    headers = {'Authorization': 'token ' + get_token()}
    logging.debug("POST: " + url)
    logging.debug("Data: " + json.dumps(data))

    if True:
        request = urllib2.Request(url, json.dumps(data), headers)
        res = urllib2.urlopen(request)
        raise_for_status(url, res)

def delete_comment(_type, _id):
    url = 'https://api.github.com/repos/{}/{}/comments/{}'.format(
          TRAVIS_REPO_SLUG, _type, _id)
    headers = {'Authorization': 'token ' + get_token()}
    logging.debug("DELETE: " + url)

    try:
        request = urllib2.Request(url, None, headers)
        request.get_method = lambda: 'DELETE'
        res = urllib2.urlopen(request)
        raise_for_status(url, res)
    except urllib2.HTTPError as e:
        print e
        print e.read()
        sys.exit(1)


def system(cmd):
    logging.debug("Running: " + " ".join(cmd))
    if isinstance(cmd, list):
        return subprocess.check_output(cmd)
    else:
        return subprocess.check_output(cmd, shell=True)


main()
