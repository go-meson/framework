#!/usr/bin/env python

import argparse
import os
import subprocess
import sys

from lib.config import LIBCHROMIUMCONTENT_COMMIT, BASE_URL, PLATFORM, \
    enable_verbose_mode, is_verbose_mode, get_target_arch
from lib.util import execute_stdout, scoped_cwd

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
VENDOR_DIR = os.path.join(SOURCE_ROOT, 'vendor')

def main():
    os.chdir(SOURCE_ROOT)

    args = parse_args()
    defines = args_to_defines(args)

    if args.verbose:
        enable_verbose_mode()

    update_submodules()

    libcc_source_path = args.libcc_source_path
    libcc_shared_library_path = args.libcc_shared_library_path
    libcc_static_library_path = args.libcc_static_library_path

    if args.build_libchromiumcontent:
        build_libchromiumcontent(args.verbose, args.target_arch)
        dist_dir = os.path.join(VENDOR_DIR, 'brightray', 'vendor',
                                'libchromiumcontent', 'dist', 'main')
        libcc_source_path = os.path.join(dist_dir, 'src')
        libcc_shared_library_path = os.path.join(dist_dir, 'shared_library')
        libcc_static_library_path = os.path.join(dist_dir, 'static_library')

    if PLATFORM != 'win32':
        # Download prebuilt clang binaries.
        update_clang()


    setup_python_libs()

    bootstrap_brightray(args.dev, BASE_URL, args.target_arch,
                        libcc_source_path, libcc_shared_library_path,
                        libcc_static_library_path)

    create_chrome_version_h()

    run_update(defines, args.msvs)

def update_submodules():
    execute_stdout(['git', 'submodule', 'sync', '--recursive'])
    execute_stdout(['git', 'submodule', 'update', '--init', '--recursive'])

def setup_python_libs():
    for lib in ['requests']:
        with scoped_cwd(os.path.join(VENDOR_DIR, lib)):
            execute_stdout([sys.executable, 'setup.py', 'build'])

def bootstrap_brightray(is_dev, url, target_arch, libcc_source_path,
                        libcc_shared_library_path,
                        libcc_static_library_path):
    bootstrap = os.path.join(VENDOR_DIR, 'brightray', 'script', 'bootstrap')
    args = [
        '--commit', LIBCHROMIUMCONTENT_COMMIT,
        '--target_arch', target_arch,
        url,
    ]
    if is_dev:
        args = ['--dev'] + args
    if (libcc_source_path != None and
        libcc_shared_library_path != None and
        libcc_static_library_path != None):
        args += ['--libcc_source_path', libcc_source_path,
                 '--libcc_shared_library_path', libcc_shared_library_path,
                 '--libcc_static_library_path', libcc_static_library_path]
    execute_stdout([sys.executable, bootstrap] + args)

def run_update(defines, msvs):
    args = [sys.executable, os.path.join(SOURCE_ROOT, 'script', 'update.py')]
    if defines:
        args += ['--defines', defines]
    if msvs:
        args += ['--msvs']

    execute_stdout(args)

def create_chrome_version_h():
    version_file = os.path.join(SOURCE_ROOT, 'vendor', 'brightray', 'vendor',
                                'libchromiumcontent', 'VERSION')
    target_file = os.path.join(SOURCE_ROOT, 'src', 'app', 'common', 'chrome_version.h')
    template_file = os.path.join(SOURCE_ROOT, 'script', 'chrome_version.h.in')
    
    with open(version_file, 'r') as f:
        version = f.read()
    with open(template_file, 'r') as f:
        template = f.read()
    content = template.replace('{PLACEHOLDER}', version.strip())
            
    # We update the file only if the content has changed (ignoring line ending
    # differences).
    should_write = True
    if os.path.isfile(target_file):
        with open(target_file, 'r') as f:
            should_write = f.read().replace('r', '') != content.replace('r', '')
    if should_write:
        with open(target_file, 'w') as f:
            f.write(content)

def build_libchromiumcontent(verbose, target_arch):
    args = [sys.executable,
            os.path.join(SOURCE_ROOT, 'script', 'build-libchromiumcontent.py')]
    if verbose:
        args += ['-v']
    
    execute_stdout(args + ['--target_arch', target_arch])

def update_clang():
    execute_stdout([os.path.join(SOURCE_ROOT, 'script', 'update_clang.sh')])

def parse_args():
    parser = argparse.ArgumentParser(description='Bootstrap this project')
    parser.add_argument('-v', '--verbose',
                        action='store_true',
                        help='Prints the output of subprocesses')
    parser.add_argument('-d', '--dev', action='store_true',
                        help='Do not download static_library build')
    parser.add_argument('--msvs', action='store_true',
                        help='Generate Visual Studio project')
    parser.add_argument('--target_arch', default=get_target_arch(),
                        help='Manually specify the arch to build for')
    parser.add_argument('--clang_dir', default='', help='Path to clang binaries')
    parser.add_argument('--disable_clang', action='store_true',
                        help='Use compilers other than clang for building')
    parser.add_argument('--build_libchromiumcontent', action='store_true',
                        help='Build local version of libchromiumcontent')
    parser.add_argument('--libcc_source_path', required=False,
                        help='The source path of libchromiumcontent. ' \
                        'NOTE: All options of libchromiumcontent are ' \
                        'required OR let electron choose it')
    parser.add_argument('--libcc_shared_library_path', required=False,
                        help='The shared library path of libchromiumcontent.')
    parser.add_argument('--libcc_static_library_path', required=False,
                        help='The static library path of libchromiumcontent.')
    parser.add_argument('--defines', default='',
                        help='The build variables passed to gyp')
    return parser.parse_args()

def args_to_defines(args):
    defines = args.defines
    if args.disable_clang:
        defines += ' clang=0'
    if args.clang_dir:
        defines += ' make_clang_dir=' + args.clang_dir
        defines += ' clang_use_chrome_plugins=0'
    return defines

if __name__ == '__main__':
    sys.exit(main())
