#!/bin/sh
# Builds a QNapi AppImage from a CMake build directory configured with
# -DCMAKE_INSTALL_PREFIX=/usr (BUILD_DIR, default: build).
# Needs a 7-Zip binary (p7zip-full or 7zip) and network access to fetch
# linuxdeploy. Set QMAKE to the qmake of the Qt the build used if it is not
# the first qmake in PATH (e.g. QMAKE=qmake6).

set -e

BUILD_DIR=${BUILD_DIR:-build}
APPDIR=${APPDIR:-AppDir}
ARCH=${ARCH:-$(uname -m)}

rm -rf "$APPDIR"
DESTDIR="$APPDIR" cmake --install "$BUILD_DIR"

# distributions install /usr/bin/7za as a shell wrapper around the binary
for P7ZIP in /usr/lib/7zip/7za /usr/lib/p7zip/7za "$(command -v 7zz)" "$(command -v 7za)"; do
  [ -x "$P7ZIP" ] && head -c 4 "$P7ZIP" | grep -q ELF && break
done
cp -v "$P7ZIP" "$APPDIR/usr/bin/7za"

for tool in linuxdeploy linuxdeploy-plugin-qt; do
  if [ ! -x "$tool-$ARCH.AppImage" ]; then
    wget -c "https://github.com/linuxdeploy/$tool/releases/download/continuous/$tool-$ARCH.AppImage"
    chmod a+x "$tool-$ARCH.AppImage"
  fi
done

unset QTDIR QT_PLUGIN_PATH LD_LIBRARY_PATH
# lets the tools run without FUSE (containers, CI)
export APPIMAGE_EXTRACT_AND_RUN=1

"./linuxdeploy-$ARCH.AppImage" \
  --appdir "$APPDIR" \
  --executable "$APPDIR/usr/bin/qnapic" \
  --executable "$APPDIR/usr/bin/7za" \
  --desktop-file "$APPDIR/usr/share/applications/qnapi.desktop" \
  --icon-file "$APPDIR/usr/share/icons/hicolor/512x512/apps/qnapi.png" \
  --plugin qt \
  --output appimage

ls -la QNapi*.AppImage
