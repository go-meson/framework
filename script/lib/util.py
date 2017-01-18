#!/usr/bin/env python

import atexit
import contextlib
import errno
import os
import subprocess
import platform
import re
import shutil
import zipfile
import sys

from config import is_verbose_mode

def get_host_arch():
    """Returns the host architecture with a predictable string."""
    host_arch = platform.machine()

    # Convert machine type to format recognized by gyp.
    if re.match(r'i.86', host_arch) or host_arch == 'i86pc':
        host_arch = 'ia32'
    elif host_arch in ['x86_64', 'amd64']:
        host_arch = 'x64'
    elif host_arch.startswith('arm'):
        host_arch = 'arm'

        # platform.machine is based on running kernel. It's possible to use 64-bit
        # kernel with 32-bit userland, e.g. to give linker slightly more memory.
        # Distinguish between different userland bitness by querying
        # the python binary.
    if host_arch == 'x64' and platform.architecture()[0] == '32bit':
        host_arch = 'ia32'

    return host_arch

def tempdir(prefix=''):
  directory = tempfile.mkdtemp(prefix=prefix)
  atexit.register(shutil.rmtree, directory)
  return directory


@contextlib.contextmanager
def scoped_cwd(path):
  cwd = os.getcwd()
  os.chdir(path)
  try:
    yield
  finally:
    os.chdir(cwd)


@contextlib.contextmanager
def scoped_env(key, value):
  origin = ''
  if key in os.environ:
    origin = os.environ[key]
  os.environ[key] = value
  try:
    yield
  finally:
    os.environ[key] = origin


def execute(argv, env=os.environ):
  if is_verbose_mode():
    print ' '.join(argv)
  try:
    output = subprocess.check_output(argv, stderr=subprocess.STDOUT, env=env)
    if is_verbose_mode():
      print output
    return output
  except subprocess.CalledProcessError as e:
    print e.output
    raise e

def execute_stdout(argv, env=os.environ):
  if is_verbose_mode():
    print ' '.join(argv)
    try:
      subprocess.check_call(argv, env=env)
    except subprocess.CalledProcessError as e:
      print e.output
      raise e
  else:
    execute(argv, env)

def make_zip(zip_file_path, files, dirs):
    safe_unlink(zip_file_path)
    if sys.platform == 'darwin':
        files += dirs
        execute(['zip', '-r', '-y', zip_file_path] + files)
    else:
        zip_file = zipfile.ZipFile(zip_file_path, "w", zipfile.ZIP_DEFLATED)
        for filename in files:
            zip_file.write(filename, filename)
        for dirname in dirs:
            for root, _, filenames in os.walk(dirname):
                for f in filenames:
                    zip_file.write(os.path.join(root, f))
        zip_file.close()


def rm_rf(path):
    try:
        shutil.rmtree(path)
    except OSError:
        pass

def safe_unlink(path):
    try:
        os.unlink(path)
    except OSError as e:
        if e.errno != errno.ENOENT:
            raise

def meson_gyp():
    SOURCE_ROOT = os.path.abspath(os.path.join(__file__, '..', '..', '..'))
    gyp = os.path.join(SOURCE_ROOT, 'meson.gyp')
    with open(gyp) as f:
        obj = eval(f.read());
        return obj['variables']

def get_meson_version():
    return 'v' + meson_gyp()['version%']

def parse_version(version):
    if version[0] == 'v':
        version = version[1:]

    vs = version.split('.')
    if len(vs) > 4:
        return vs[0:4]
    else:
        return vs + ['0'] * (4 - len(vs))

def import_vs_env(target_arch):
  if sys.platform != 'win32':
    return

  if target_arch == 'ia32':
    vs_arch = 'amd64_x86'
  else:
    vs_arch = 'x86_amd64'
  env = get_vs_env('14.0', vs_arch)
  os.environ.update(env)
