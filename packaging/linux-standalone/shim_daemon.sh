#!/bin/sh
HERE=$(dirname "$(readlink -f "${0}")")
export PATH="${HERE}:${PATH}"

exec "$HERE/elf/ld-linux.so" --library-path "$HERE/../lib" "$HERE/elf/librevault-daemon" "$@"
