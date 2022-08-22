#!/usr/bin/env python

"""
Unpacks an MPQ and minifies its assets.

1. All graphics assets are converted to CLX.

Dependencies:
- smpq
- pcx2clx cel2clx cl22clx https://github.com/diasurgical/devilutionx-graphics-tools
- lame if converting audio to mp3 (optional)
"""

import argparse
from concurrent import futures
import glob
import logging
import os
import sys
import subprocess

_LOGGER = logging.getLogger()
_LOGGER.setLevel(logging.INFO)
_LOGGER.addHandler(logging.StreamHandler(sys.stderr))


def get_mpq_name(mpq_path: str) -> str:
    return os.path.splitext(os.path.basename(mpq_path))[0].lower()


def get_dest_name(mpq_name: str) -> str:
    if mpq_name == 'hfmonk' or mpq_name == 'hfmusic' or mpq_name == 'hfvoice':
        return 'hellfire'
    return mpq_name


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
    with open(listfile_path) as f:
        files = f.read().splitlines()
    with open(get_files_to_remove_path(get_mpq_name(mpq_abs_path))) as f:
        to_remove = f.read().replace('/', '\\').splitlines()
    files = sorted(list(set(files) - set(to_remove)))
    run('smpq', '--extract', mpq_abs_path, *files)


def convert_to_clx(mpq_name: str):
    with open(get_clx_commands_path(mpq_name)) as f:
        for line in f:
            if line.startswith('#'):
                continue
            args = line.split()
            args.insert(1, '--remove')
            run(*args)


def convert_wav_to_mp3(path: str):
    run('lame', '--replaygain-accurate', '--quiet', '-q',
        '0', path, f'{os.path.splitext(path)[0]}.mp3')
    os.remove(path)


def convert_to_mp3():
    files = glob.glob('**/*.wav', recursive=True)
    try:
        with futures.ThreadPoolExecutor(max_workers=os.cpu_count()) as executor:
            executor.map(convert_wav_to_mp3, files,
                         chunksize=len(files) / os.cpu_count())
    except BaseException as e:
        logging.error(str(e))
        exit(1)


def convert(mpq_path: str, listfile_path: str, output_dir: str, mp3: bool):
    _LOGGER.info(f"Extracting {mpq_path} to {output_dir}")
    mpq_abs_path = os.path.abspath(mpq_path)
    mpq_name = get_mpq_name(mpq_path)
    os.makedirs(output_dir, exist_ok=True)
    cwd = os.getcwd()
    os.chdir(output_dir)
    unpack(listfile_path, mpq_abs_path)
    convert_to_clx(mpq_name)
    if mp3:
        convert_to_mp3()
    os.chdir(cwd)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output-dir')
    parser.add_argument('--listfile')
    parser.add_argument('--mp3', default=False, action='store_true')
    parser.add_argument('mpq', nargs='+')
    args = parser.parse_args()
    for mpq_path in args.mpq:
        mpq_name = get_mpq_name(mpq_path)
        if args.output_dir:
            output_dir = args.output_dir
        else:
            output_dir = os.path.join(os.getcwd(), 'output', get_dest_name(mpq_name))
        if args.listfile:
            listfile = args.listfile
        else:
            listfile = get_builtin_listfile_path(mpq_name)
        convert(mpq_path, listfile, output_dir, args.mp3)


main()
