#!/usr/bin/env python3

import sys

args = sys.argv[1:]

for line in sys.stdin:
    for argN in range(0, len(args)-1, 2):
        line = line.replace(args[argN], args[argN+1])
    print(line, end='')
