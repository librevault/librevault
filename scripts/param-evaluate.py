#!/usr/bin/env python3
import sys

for line in sys.stdin:
    for param, value in zip(sys.argv[1::2], sys.argv[2::2]):
        line = line.replace(param, value)
    print(line, end='')
