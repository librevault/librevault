#!/bin/bash

cd "$(dirname $0)/.."

ROOT_DIR="$(mktemp -d /tmp/flatpak-builder-XXXXXXX)"

FLATPAK_PIP_GENERATOR="${ROOT_DIR}/flatpak-pip-generator"

curl -LO https://raw.githubusercontent.com/flatpak/flatpak-builder-tools/master/pip/flatpak-pip-generator > ${FLATPAK_PIP_GENERATOR}
chmod +x ${FLATPAK_PIP_GENERATOR}

REQUIREMENTS="${ROOT_DIR}/requirements.txt"
poetry export -f requirements.txt --without-hashes > ${REQUIREMENTS}

${FLATPAK_PIP_GENERATOR} --requirements-file="${REQUIREMENTS}" --output pypi-dependencies
