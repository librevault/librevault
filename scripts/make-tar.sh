#!/bin/bash
ORIG_DIR="$(pwd)"

TOOL_DIR=$(mktemp -d)
TOOL_GIT_ARCHIVE_ALL=$(echo "$TOOL_DIR/git-archive-all.sh")
cd "$(dirname $0)/.."

git submodule update --init --recursive
wget https://raw.githubusercontent.com/Librevault/git-archive-all.sh/master/git-archive-all.sh -O "$TOOL_GIT_ARCHIVE_ALL"
chmod +x "$TOOL_GIT_ARCHIVE_ALL"

TEMP_ARCHIVE="$ORIG_DIR/librevault_$(git describe).tar"
$TOOL_GIT_ARCHIVE_ALL --format tar "$TEMP_ARCHIVE"
echo -n `git describe` > $TOOL_DIR/VERSION
cd "$TOOL_DIR"
tar -rf "$TEMP_ARCHIVE" VERSION
gzip "$TEMP_ARCHIVE"
