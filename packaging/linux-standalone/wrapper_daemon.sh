#!/bin/sh

export LD_LIBRARY_PATH="/opt/librevault/lib"
export PATH="/opt/librevault/bin:${PATH}"

exec /opt/librevault/bin/librevault-daemon "$@"
