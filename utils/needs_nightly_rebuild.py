#!/usr/bin/env python3
import urllib.request, urllib.parse, urllib.error
import urllib.request, urllib.error, urllib.parse
import sys
import os
import json
import subprocess

TRAVIS = os.environ.get('TRAVIS')
TRAVIS_REPO_SLUG = os.environ.get('TRAVIS_REPO_SLUG')
TRAVIS_BRANCH = os.environ.get('TRAVIS_BRANCH')

def main():
    if not TRAVIS or not TRAVIS_REPO_SLUG:
        print("Not a Travis environment.  No rebuild needed")
        sys.exit(1)
    sha = subprocess.check_output(['git', 'rev-parse', 'HEAD']).decode('utf-8').rstrip()
    if not sha:
        print("Could not determine current git commit.  No rebuild needed")
        sys.exit(1)
    builds_url = ('https://api.travis-ci.org/repo/'
                  + urllib.parse.quote_plus(TRAVIS_REPO_SLUG)
                  + "/builds?limit=5&state=passed&branch.name="
                  + TRAVIS_BRANCH
                  + "&event_type=")

    headers = {"Travis-API-Version": 3}
    for event in ['cron']:  # 'api'  Only checking cron jobs.  api jobs will force a rebuild
        url = builds_url + event
        try:
            request = urllib.request.Request(url, None, headers)
            res = urllib.request.urlopen(request)
            raise_for_status(url, res)
            data = json.loads(res.read())
        except:
            print("Couldn't fetch build data.  Assuming rebuild needed")
            sys.exit(0)
        if not data or not isinstance(data.get('builds'), list):
            print("Couldn't parse build data.  Assuming rebuild needed")
            sys.exit(0)
        if any(build['commit']['sha'] == sha for build in data['builds']):
            print("Found previous nightly build for {}.  Skipping rebuild".format(sha[:8]))
            sys.exit(1)
    print("No nightly builds matched {}.  Assuming rebuild needed".format(sha[:8]))

def raise_for_status(url, response):
    """Raise exception on URL error"""
    if response.getcode() < 200 or response.getcode() >= 300:
        sys.stderr.write(response.read())
        sys.stderr.write('\n')
        raise Exception('Request for %s failed: %s' % (url, response.getcode()))

main()
