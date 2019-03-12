#!/usr/bin/env python3
"""Track changes in the size of an image using Travis-CI and GitHub statuses.

This code is adapted from https://github.com/danvk/travis-weigh-in

Usage:
    archive_build_size.py <MAKETARGET>

This should be run inside of a Travis-CI worker.
It will post the current size of the file as a GitHub status on the commit.

If it's able to deduce the size of the file before the change, it will report
the size delta. This requires that this script was run on the base for a Pull
Request, e.g. the commit that was merged into master.
"""

import os
import re
import sys
import json
import urllib.request, urllib.error, urllib.parse
import zlib
import binascii
from collections import namedtuple

TRAVIS = os.environ.get('TRAVIS')
TRAVIS_COMMIT = os.environ.get('TRAVIS_COMMIT')
TRAVIS_PULL_REQUEST = os.environ.get('TRAVIS_PULL_REQUEST')
TRAVIS_REPO_SLUG = os.environ.get('TRAVIS_REPO_SLUG')
GITHUB_TOKEN = os.environ.get('GITHUB_TOKEN')

ImageSize = namedtuple('ImageSize', ['rom', 'ram'])

if TRAVIS_PULL_REQUEST == 'false':
    TRAVIS_PULL_REQUEST = False

# The PR job has all the needed info to compute deltas.
# But GitHub shows statuses for the commit associated with the push job.
# For the PR job, this will be the status URL for the push job.
TRAVIS_STATUS_URL = None


def raise_for_status(url, response):
    """Raise exception on URL error"""
    if response.getcode() < 200 or response.getcode() >= 300:
        sys.stderr.write(response.read())
        sys.stderr.write('\n')
        raise Exception('Request for %s failed: %s' % (url, response.getcode()))


def get_token():
    """Return github token"""
    token = zlib.decompress(binascii.unhexlify(GITHUB_TOKEN))
    length = len(token)
    return ''.join(chr(a ^ b)
                   for a, b in zip(token, (TRAVIS_REPO_SLUG.encode('utf-8') * length)[:length]))


def post_status(url, state, context, description):
    """Send status update to GitHub"""
    data = {
        'state': state,
        'context': context,
        'description': description
    }
    headers = {'Authorization': 'token ' + get_token()}

    request = urllib.request.Request(url, json.dumps(data).encode('utf-8'), headers)
    res = urllib.request.urlopen(request)
    raise_for_status(url, res)

    print('Posted %s' % json.dumps(data))


def get_status(url, context):
    """Request status for previous build from GitHub"""
    headers = {'Authorization': 'token ' + get_token()}
    request = urllib.request.Request(url, None, headers)
    res = urllib.request.urlopen(request)
    raise_for_status(url, res)

    data = json.loads(res.read().decode('utf-8'))
    for status in data:
        if status['context'] == context:
            return status['description']
    for status in data:
        # Legacy support
        if status['context'] == "image_size/" + context:
            return status['description']
    return None


def get_pr_info(slug, pull_number):
    """Retireve pull-request info"""
    url = 'https://api.github.com/repos/%s/pulls/%s' % (slug, pull_number)
    headers = {'Authorization': 'token ' + get_token()}
    request = urllib.request.Request(url, None, headers)
    res = urllib.request.urlopen(request)
    raise_for_status(url, res)
    return json.loads(res.read().decode('utf-8'))


def get_context(filename):
    """Define status context"""
    return filename


def get_base_size(filename):
    """Retrieve previous image size for base of pull-request"""
    global TRAVIS_STATUS_URL
    if not TRAVIS_PULL_REQUEST:
        return None
    pullreq = get_pr_info(TRAVIS_REPO_SLUG, TRAVIS_PULL_REQUEST)
    sha = pullreq['base']['sha']
    url = pullreq['base']['repo']['statuses_url'].replace('{sha}', sha)
    TRAVIS_STATUS_URL = pullreq['statuses_url']
    assert sha in url, 'statuses_url %s missing "{sha}"' % url
    status = get_status(url, get_context(filename))
    if not status:
        sys.stderr.write('Unable to find status %s for base at %s\n' % (filename, url))
        return None
    return parse_description(status)


def format_description(current_size, previous_size):
    """Format string defining current image size info"""
    if previous_size:
        rom_delta = current_size.rom - previous_size.rom
        ram_delta = current_size.ram - previous_size.ram
        return (
            'ROM: {:,.0f} \u0394: {:+,.0f}b/{:+0.2f}% '
            'RAM: {:,.0f} \u0394: {:+,.0f}b/{:+0.2f}%'.format(
                current_size.rom, rom_delta, 100.0 * rom_delta /  current_size.rom,
                current_size.ram, ram_delta, 100.0 * ram_delta /  current_size.ram))

    return 'ROM: {:,.0f} RAM: {:,.0f}'.format(current_size.rom, current_size.ram)


def parse_description(description):
    """Parse size info from existsing status"""
      
    match = re.search(r'ROM:\s*([0-9,]+).*\sRAM:\s*([0-9,]+)', description)
    assert match, 'Unable to parse "%s"' % description
    rom_size = int(match.group(1).replace(',', ''))
    ram_size = int(match.group(2).replace(',', ''))
    return ImageSize(rom=rom_size, ram=ram_size)


def check_environment():
    """Verify script is running in TravisCI and has needed variables"""
    if not GITHUB_TOKEN:
        sys.stderr.write('The GITHUB_TOKEN environment variable must be set.\n')
        sys.exit(1)

    if not TRAVIS:
        sys.stderr.write('Not Travis; exiting\n')
        sys.exit(0)

    if not TRAVIS_COMMIT:
        sys.stderr.write('Missing TRAVIS_COMMIT\n')
        sys.exit(1)

    if not TRAVIS_REPO_SLUG:
        sys.stderr.write('Missing TRAVIS_REPO_SLUG\n')
        sys.exit(1)


def get_image_size(image):
    """Read ROM and RAM size from map file"""
    rom_start = 0
    rom_end = 0
    data_end = 0
    ram_start = 0
    ram_end = 0
    with open(image, "r") as _fh:
        for line in _fh:
            match = re.search(r'^\.data\s+(0x\S+)', line)
            if match:
                ram_start = int(match.group(1), 16)
                continue
            match = re.search(r'(0x\S+)\s+_ebss = \.', line)
            if match:
                ram_end = int(match.group(1), 16)
                continue
            match = re.search(r'(0x\S+)\s+_etext = \.', line)
            if match:
                rom_end = int(match.group(1), 16)
                continue
            match = re.search(r'^\.text\s+(0x\S+)', line)
            if match:
                rom_start = int(match.group(1), 16)
                continue
            match = re.search(r'(0x\S+)\s+_edata = \.', line)
            if match:
                data_end = int(match.group(1), 16)
                continue
    rom_end += data_end - ram_start
    return ImageSize(rom=rom_end-rom_start, ram=ram_end-ram_start)

def main():
    """Main function"""
    if len(sys.argv) != 2:
        sys.stderr.write('Usage: %s path/to/mapfile\n' % sys.argv[0])
        sys.exit(1)

    filename = sys.argv[1].replace("zip_", "").replace("win_", "") + ".map"
    if not os.path.exists(filename):
        print("No map file generated ({}): {}".format(sys.argv[1], filename))
        return

    check_environment()

    current_size = get_image_size(filename)
    previous_size = get_base_size(filename)

    print('%s Current:  ROM: %s RAM: %s' % (filename, current_size.rom, current_size.ram))
    if previous_size:
        print('%s Previous: ROM: %s RAM: %s' % (filename, previous_size.rom, previous_size.ram))

    if TRAVIS_STATUS_URL:
        url = TRAVIS_STATUS_URL
    else:
        url = 'https://api.github.com/repos/%s/statuses/%s' % (TRAVIS_REPO_SLUG, TRAVIS_COMMIT)

    print('POSTing to %s' % url)
    post_status(url, 'success', get_context(filename),
                format_description(current_size, previous_size))


if __name__ == '__main__':
    main()
