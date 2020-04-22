import fileinput
import os
from distutils.dir_util import copy_tree
from pathlib import Path
from typing import Dict

import click

from lib.makeversion import versioninfo


def configure_file(path: Path, params: Dict[str, str]):
    with fileinput.FileInput(path, inplace=True, backup=".bak") as f:
        for line in f:
            for k, v in params.items():
                line = line.replace(k, v)
            print(line, end="")


@click.command()
@click.option("--packaging_dir")
@click.option("--install_dir")
def generate(packaging_dir, install_dir):
    innosetup_dir = Path(__file__).resolve().parent.parent / "packaging" / "innosetup"
    packaging_dir = Path(packaging_dir)

    copy_tree(innosetup_dir, str(packaging_dir))
    os.makedirs(packaging_dir / "package", exist_ok=True)
    copy_tree(install_dir, str(packaging_dir / "package"))
    configure_file(packaging_dir / "librevault.iss", {"@librevault_VERSION@": versioninfo()})


if __name__ == "__main__":
    generate()
