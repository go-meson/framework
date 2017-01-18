#!/usr/bin/env python

import argparse
import errno
import hashlib
import os
import shutil
import subprocess
import sys
import tempfile

from io import StringIO
from lib.config import PLATFORM, get_target_arch, get_chromedriver_version, \
    get_env_var, get_zip_name
from lib.util import meson_gyp, execute, get_meson_version, \
    parse_version, scoped_cwd
from lib.github import GitHub

MESON_REPO = 'go-meson/framework'
MESON_VERSION = get_meson_version()

PROJECT_NAME = meson_gyp()['project_name%']
PRODUCT_NAME = meson_gyp()['product_name%']

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
OUT_DIR = os.path.join(SOURCE_ROOT, 'out', 'R')
DIST_DIR = os.path.join(SOURCE_ROOT, 'dist')

DIST_NAME = get_zip_name(PROJECT_NAME, MESON_VERSION)
SYMBOLS_NAME = get_zip_name(PROJECT_NAME, MESON_VERSION, 'symbols')
DSYM_NAME = get_zip_name(PROJECT_NAME, MESON_VERSION, 'dsym')
PDB_NAME = get_zip_name(PROJECT_NAME, MESON_VERSION, 'pdb')



def main():
    args = parse_args()

    github = GitHub(auth_token())
    releases = github.repos(MESON_REPO).releases.get()
    tag_exists = False
    for release in releases:
        if not release['draft'] and release['tag_name'] == args.version:
            tag_exists = True
            break

    release = create_or_get_release_draft(github, releases, args.version, tag_exists)

    upload_meson(github, release, os.path.join(DIST_DIR, DIST_NAME))


def upload_meson(github, release, file_path):
    filename = os.path.basename(file_path)
    if os.environ.has_key('CI'):
        try:
            for asset in release['assets']:
                if asset['name'] == filename:
                    github.repos(MESON_REPO).releases.assets(asset['id']).delete()
        except Exception:
            pass

    # Upload the file.
    with open(file_path, 'rb') as f:
        upload_io_to_github(github, release, filename, f, 'application/zip')

    # Upload the checksum file.
    upload_sha256_checksum(github, release, file_path)

def upload_io_to_github(github, release, name, io, content_type):
    params = {'name': name}
    headers = {'Content-Type': content_type}
    github.repos(MESON_REPO).releases(release['id']).assets.post(
        params=params, headers=headers, data=io, verify=False)

def upload_sha256_checksum(github, release, file_path):
    checksum_path = '{}.sha256sum'.format(file_path)
    sha256 = hashlib.sha256()
    with open(file_path, 'rb') as f:
        sha256.update(f.read())

    filename = os.path.basename(checksum_path)
    checksum = sha256.hexdigest()
    upload_io_to_github(github, release, filename,
                        StringIO(checksum.decode('utf-8')), 'text/plain')


def get_text_with_editor(name):
    editor = os.environ.get('EDITOR', 'nano')
    initial_message = '\n# Please enter the body of your release note for %s.' \
                      % name
    t = tempfile.NamedTemporaryFile(suffix='.tmp', delete=False)
    t.write(initial_message)
    t.close()
    subprocess.call([editor, t.name])

    text = ''
    for line in open(t.name, 'r'):
        if len(line) == 0 or line[0] != '#':
            text += line
    
    os.unlink(t.name)
    return text

def create_or_get_release_draft(github, releases, tag, tag_exists):
    for release in releases:
        if release['draft']:
            return release
    if tag_exists:
        tag = 'do-not-publish-me'
    return create_release_draft(github, tag)

def create_release_draft(github, tag):
    name = '{0} {1}'.format(PROJECT_NAME, tag)
    if os.environ.has_key('CI'):
        body = '(placeholder)'
    else:
        body = get_text_with_editor(name)
    if body == '':
        sys.stderr.write('QUit due to empty release note.\n')
        sys.exit(0)
    data = dict(tag_name=tag, name=name, body=body, draft=True)
    r = github.repos(MESON_REPO).releases.post(data=data)
    return r

def auth_token():
    token = get_env_var('GITHUB_TOKEN')
    message = ('Error: Please set the $MESON_GITHUB_TOKEN '
               'environment variable, which is your personal token')
    assert token, message
    return token

def parse_args():
    parser = argparse.ArgumentParser(description='upload distribution file')
    parser.add_argument('-v', '--version', help='Specify the version',
                        default=MESON_VERSION)
    parser.add_argument('-p', '--publish-release',
                        help='Publish the release',
                        action='store_true')
    return parser.parse_args()

if __name__ == '__main__':
  import sys
  sys.exit(main())

