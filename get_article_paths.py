# -*- coding: utf-8 -*-

import sys
import os


def main():
    dest = sys.argv[1]
    src = sys.argv[2:]

    fout = open(dest, 'w')

    for dir in src:
        for file in os.listdir(dir):
            print(file, dir, file=fout)

    fout.close()

if __name__ == '__main__':
    main()