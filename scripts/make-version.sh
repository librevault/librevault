#!/bin/bash
if [ -f "$(dirname $0)/../VERSION" ]; then
    VERSION_STRING="$(cat "$(dirname $0)/../VERSION")"
else
    cd "$(dirname $0)/.."
    VERSION_STRING="$(git describe)"
fi

case "$1" in
"debian")
    echo $VERSION_STRING | cut -b 2- | sed "s/-/+/" | sed "s/-/~/"
    ;;
*)
    echo "$VERSION_STRING"
    ;;
esac
