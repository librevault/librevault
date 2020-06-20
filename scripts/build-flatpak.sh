#!/bin/bash
set -xe

cd "$(dirname $0)/.."

ROOT_DIR="$(mktemp -d /tmp/flatpak-builder-XXXXXXX)"

FLATPAK_POETRY_GENERATOR="${ROOT_DIR}/flatpak-poetry-generator"

curl -L -o "${FLATPAK_POETRY_GENERATOR}" https://raw.githubusercontent.com/flatpak/flatpak-builder-tools/master/poetry/flatpak-poetry-generator.py
chmod +x "${FLATPAK_POETRY_GENERATOR}"

${FLATPAK_POETRY_GENERATOR} "poetry.lock" -o flatpak/python3-conan.json
