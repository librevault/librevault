import subprocess
import re
from pathlib import Path


def get_version_string():
    try:
        with open(Path(__file__).resolve().parent.parent.parent / "VERSION") as f:
            return f.readline().strip()
    except FileNotFoundError:
        out = subprocess.run(["git", "describe"], stdout=subprocess.PIPE)
        return out.stdout.decode().strip()[1:]


def versioninfo():
    matches = re.match(r'^(\d+\.\d+\.\d+)(?:-(\d+))?', get_version_string())
    return f"{matches[0]}.0"


# if len(sys.argv) > 1 and sys.argv[1] == 'versionstring':
#     version_string = version_string[1:]
# elif len(sys.argv) > 1 and sys.argv[1] == 'versioninfo':
#     matches = re.match(r'^v(\d+)\.(\d+)\.(\d+)(?:-(\d+))?', version_string)
#     groups = matches.groups("0")
#     version_string = '.'.join(groups)
# elif len(sys.argv) > 1 and sys.argv[1] == 'debian':
#     matches = re.match(r'^v(\d+)\.(\d+)\.(\d+)(?:-(\d+)-g(\w+))?', version_string)
#     groups = matches.groups("0")
#     if groups[3] == "0":
#         version_string = groups[0] + "." + groups[1] + "." + groups[2]
#     else:
#         version_string = groups[0] + "." + groups[1] + "." + groups[2] + "+" + groups[3] + "~" + groups[4]
#
# print(version_string)
