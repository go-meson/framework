#!/usr/bin/env python

import os
import sys

from lib.util import get_meson_version, parse_version

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
MESON_VERSION = get_meson_version()

def create_framework_version_h():
    template_file = os.path.join(SOURCE_ROOT, 'script', 'meson_version_h.in')
    target_file = os.path.join(SOURCE_ROOT, 'src', 'api', 'version.h')

    vers = parse_version(MESON_VERSION.strip())

    with open(template_file, 'r') as f:
        template = f.read()
    content = template.replace('{PLACEHOLDER_VERSION_MAJOR}', vers[0])
    content = content.replace('{PLACEHOLDER_VERSION_MINOR}', vers[1])
    content = content.replace('{PLACEHOLDER_VERSION_REVISION}', vers[2])

    should_write = True
    if os.path.isfile(target_file):
        with open(target_file, 'r') as f:
            should_write = f.read() != content
    if should_write:
        with open(target_file, 'w') as f:
            f.write(content)

if __name__ == '__main__':
    sys.exit(create_framework_version_h())
