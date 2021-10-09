#!/usr/bin/env bash
cd "$(dirname $0)/../build" || exit 1

WGET_ARGS=(--continue --no-verbose)

CMAKE_ROOT="../cmake-build-debug"

curl -L "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" -O "linuxdeploy-x86_64.AppImage" &&
  set -- "$@" --output appimage

curl -L "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage" -O "linuxdeploy-plugin-qt-x86_64.AppImage" &&
  set -- "$@" --plugin qt

chmod --verbose a+x ./*.AppImage

export QMAKE=/home/gamepad/.conan/data/qt/5.15.2/_/_/package/a2079d51a8e3811d429eac8d02e7a8437cfbd22e/bin/qmake

./linuxdeploy-x86_64.AppImage \
  -e "${CMAKE_ROOT}/bin/librevault-daemon" \
  -e "${CMAKE_ROOT}/bin/librevault-gui" \
  --plugin qt \
  --appdir "AppDir" \
  --output appimage \
  -i "../packaging/appimage/librevault.svg" \
  --desktop-file "../src/gui/resources/Librevault.desktop"
