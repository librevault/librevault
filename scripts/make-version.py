#!/usr/bin/env python3

import subprocess
import re
import sys
import os

os.chdir(sys.path[0])

try:
    version_file = open("../VERSION")
    version_string = version_file.readline().strip()
except FileNotFoundError:
    out = subprocess.run(["git", "describe"], stdout=subprocess.PIPE)
    version_string = out.stdout.decode().strip()

if len(sys.argv) > 1 and sys.argv[1] == 'versionstring':
    version_string = version_string[1:]
elif len(sys.argv) > 1 and sys.argv[1] == 'versioninfo':
    matches = re.match(r'^v(\d+)\.(\d+)\.(\d+)(?:-(\d+))?', version_string)
    groups = matches.groups("0")
    version_string = '.'.join(groups)
elif len(sys.argv) > 1 and sys.argv[1] == 'debian':
    matches = re.match(r'^v(\d+)\.(\d+)\.(\d+)(?:-(\d+)-g(\w+))?', version_string)
    groups = matches.groups("0")
    if groups[3] == "0":
        version_string = groups[0] + "." + groups[1] + "." + groups[2]
    else:
        version_string = groups[0] + "." + groups[1] + "." + groups[2] + "+" + groups[3] + "~" + groups[4]

print(version_string)
