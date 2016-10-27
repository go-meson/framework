#!/usr/bin/env python

import os
import subprocess
import platform
import re

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
