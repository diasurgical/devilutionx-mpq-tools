#!/usr/bin/env python

"""
Unpacks an MPQ and minifies its assets.

1. All graphics assets are converted to CLX.

Dependencies:
- smpq
- pcx2clx cel2clx cl22clx https://github.com/diasurgical/devilutionx-graphics-tools
"""

import argparse
import logging
import os
import sys
import subprocess

_LOGGER = logging.getLogger()
_LOGGER.setLevel(logging.INFO)
_LOGGER.addHandler(logging.StreamHandler(sys.stderr))


def get_mpq_name(mpq_path: str) -> str:
    return os.path.splitext(os.path.basename(mpq_path))[0].lower()


def get_builtin_listfile_path(mpq_name: str) -> str:
    return os.path.normpath(os.path.join(os.path.dirname(__file__), 'data', f'{mpq_name}-listfile.txt'))


def get_clx_commands_path(mpq_name: str) -> str:
    return os.path.normpath(os.path.join(os.path.dirname(__file__), 'data', f'{mpq_name}-clx.txt'))


def get_files_to_remove_path(mpq_name: str) -> str:
    return os.path.normpath(os.path.join(os.path.dirname(__file__), 'data', f'{mpq_name}-rm.txt'))


def run(*args):
    _LOGGER.info(f'+ {subprocess.list2cmdline(args)}')
    result = subprocess.run(args, stdout=sys.stdout, stderr=sys.stderr)
    if result.returncode != 0:
        _LOGGER.error(f"Subprocess exit code {result.returncode}")
        exit(result.returncode)


def unpack(listfile_path: str, mpq_abs_path: str):
    run('smpq', '--listfile', listfile_path, '--extract', mpq_abs_path)


def convert_to_clx(mpq_name: str):
    with open(get_clx_commands_path(mpq_name)) as f:
        for line in f:
            args = line.split()
            args.insert(1, '--remove')
            run(*args)


def remove_unused(mpq_name: str):
    with open(get_files_to_remove_path(mpq_name)) as f:
        for line in f:
            os.remove(line.removesuffix('\n'))


def convert(mpq_path: str, listfile_path: str, output_dir: str):
    _LOGGER.info(f"Extracting {mpq_path} to {output_dir}")
    mpq_abs_path = os.path.abspath(mpq_path)
    mpq_name = get_mpq_name(mpq_path)
    os.makedirs(output_dir, exist_ok=True)
    os.chdir(output_dir)
    unpack(listfile_path, mpq_abs_path)
    convert_to_clx(mpq_name)
    remove_unused(mpq_name)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output-dir')
    parser.add_argument('--listfile')
    parser.add_argument('mpq', nargs='+')
    args = parser.parse_args()
    for mpq_path in args.mpq:
        mpq_name = get_mpq_name(mpq_path)
        if args.output_dir:
            output_dir = args.output_dir
        else:
            output_dir = os.path.join(os.getcwd(), 'output', mpq_name)
        if args.listfile:
            listfile = args.listfile
        else:
            listfile = get_builtin_listfile_path(mpq_name)
        convert(mpq_path, listfile, output_dir)


main()
