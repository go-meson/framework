#!/usr/bin/env python

import argparse
import os
import subprocess
import sys

from lib.config import get_target_arch, PLATFORM
from lib.util import meson_gyp, import_vs_env


CONFIGURATIONS = ['Release', 'Debug']
SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
PRODUCT_NAME = meson_gyp()['product_name%']

BUILD_TARGETS = {
    'darwin': [
        '{0}.framework'.format(PRODUCT_NAME),
        '{0} Helper.app'.format(PRODUCT_NAME),
    ],
    'win32': [
        PRODUCT_NAME,
    ],
    'linux': [
        PRODUCT_NAME,
    ],
}

def main():
  os.chdir(SOURCE_ROOT)

  # Update the VS build env.
  import_vs_env(get_target_arch())

  ninja = os.path.join('vendor', 'depot_tools', 'ninja')
  if sys.platform == 'win32':
    ninja += '.exe'

  args = parse_args()
  for config in args.configuration:
    build_path = os.path.join('out', config[0])
    build_targets = BUILD_TARGETS[PLATFORM]
    if args.target != '':
        build_targets = [args.target]

    for build_target in build_targets:
      cmds = [ninja, '-C', build_path, build_target]
      if args.verbose:
        cmds.append('-v')
      ret = subprocess.call(cmds)
      if ret != 0:
        sys.exit(ret)


def parse_args():
  parser = argparse.ArgumentParser(description='Build project')
  parser.add_argument('-c', '--configuration',
                      help='Build with Release or Debug configuration',
                      nargs='+',
                      default=CONFIGURATIONS,
                      required=False)
  parser.add_argument('-t', '--target',
                      help='Build specified target',
                      default='',
                      required=False)
  parser.add_argument('-v', '--verbose',
                      action='store_true',
                      help='Prints the output of subprocess')
  return parser.parse_args()


if __name__ == '__main__':
  sys.exit(main())
