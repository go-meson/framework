#!/usr/bin/env python

import sys
import os
import errno

BASE_URL = 'https://s3.amazonaws.com/github-janky-artifacts/libchromiumcontent'
LIBCHROMIUMCONTENT_COMMIT = '4f5b89374df7ee69095b9f7d50b30fb46ddd7407'

PLATFORM = {
    'cygwin': 'win32',
    'darwin': 'darwin',
    'linux2': 'linux',
    'win32': 'win32',
}[sys.platform]

verbose_mode = False

def get_platform_key():
  if os.environ.has_key('MAS_BUILD'):
    return 'mas'
  else:
    return PLATFORM

def get_target_arch():
  try:
    target_arch_path = os.path.join(__file__, '..', '..', '..', 'vendor',
                                    'brightray', 'vendor', 'download',
                                    'libchromiumcontent', '.target_arch')
    with open(os.path.normpath(target_arch_path)) as f:
      return f.read().strip()
  except IOError as e:
    if e.errno != errno.ENOENT:
      raise

  return 'x64'

def enable_verbose_mode():
  print 'Running in verbose mode'
  global verbose_mode
  verbose_mode = True


def is_verbose_mode():
  return verbose_mode

def get_chromedriver_version():
  return 'v2.21'

def get_env_var(name):
  value = os.environ.get('MESON_' + name, '')
  return value

def get_zip_name(name, version, suffix=''):
  arch = get_target_arch()
  if arch == 'arm':
    arch += 'v7l'
  zip_name = '{0}-{1}-{2}-{3}'.format(name, version, get_platform_key(), arch)
  if suffix:
    zip_name += '-' + suffix
  return zip_name + '.zip'
