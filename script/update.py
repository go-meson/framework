#!/usr/bin/env python

import os
import sys
import subprocess

from lib.config import get_target_arch
from lib.util import get_host_arch

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))

def main():
    os.chdir(SOURCE_ROOT)
    return update_gyp()

def update_gyp():
    target_arch = get_target_arch()
    #return (run_gyp(target_arch, 0) or run_gyp(target_arch, 1))
    return run_gyp(target_arch, 1)

def run_gyp(target_arch, component):
    env = os.environ.copy()
    python = sys.executable
    gyp = os.path.join('vendor', 'brightray', 'vendor', 'gyp', 'gyp_main.py')
    gyp_pylib = os.path.join(os.path.dirname(gyp), 'pylib')
    # Avoid using the old gyp lib in system.
    env['PYTHONPATH'] = os.path.pathsep.join([gyp_pylib,
                                              env.get('PYTHONPATH', '')])
    # Whether to build for Mac App Store.
    if os.environ.has_key('MAS_BUILD'):
        mas_build = 1
    else:
        mas_build = 0

    defines = [
        '-Dlibchromiumcontent_component={0}'.format(component),
        '-Dtarget_arch={0}'.format(target_arch),
        '-Dhost_arch={0}'.format(get_host_arch()),
        '-Dlibrary=static_library',
        '-Dmas_build={0}'.format(mas_build),
    ]

    generator = 'ninja'
    return subprocess.call([python, gyp, '-f', generator, '--depth', '.',
                            'meson.gyp', '-Icommon.gypi'] + defines, env=env)

if __name__ == '__main__':
    sys.exit(main())
