#!/usr/bin/python3

import subprocess
import sys
import shutil
import os

ldd = '/usr/bin/ldd'

def main(argv: list[str]) -> int:
    argv_len = len(argv)
    do_copy = argv_len == 3
    if not argv_len in [2, 3]:
        print(f"Arguments required: {argv[0]} <input_file> <output_dir>")
        return -1

    input_file: str = argv[1]
    output_dir: str = argv[2] if do_copy else ''

    if not os.path.isfile(input_file):
        print(f"Input file: '{input_file}' is not a file.")
        return -1
    if do_copy:
        if not os.path.isdir(output_dir):
            print(f"Output directory: '{output_dir}' is not a directory.")
            return -1
    else:
        print(f"`{input_file}` linked libraries:")

    ret = subprocess.run([ldd, input_file], stdout=subprocess.PIPE)
    if ret.returncode != 0:
        print(f"'{ldd}' failed with exit code: {ret.returncode}")
        return ret.returncode

    output = ret.stdout.decode()

    for line in output.split('\n'):
        if line.find('ld-linux-x86-64') != -1:
            continue
        line_split = line.split(" => ")
        if len(line_split) == 2:
            path = line_split[1].split(' ')[0]
            if do_copy:
                print(f"Copying '{path}' to '{output_dir}'...")
                shutil.copy2(path, output_dir)
            else:
                print(f"\t{path}")
    return 0

if __name__ == '__main__':
    ret = main(sys.argv)
    sys.exit(ret)
