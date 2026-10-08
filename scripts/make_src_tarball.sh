#!/bin/sh
# Creates ../qnapi-$VERSION.tar.gz from the committed sources, without the
# prebuilt third-party binaries. VERSION defaults to the one in version.h.

set -e

cd "$(dirname "$0")/.."
VERSION=${VERSION:-$(sh scripts/version.sh)}
DST_DIR=qnapi-$VERSION
TMP_DIR=$(mktemp -d)

git archive --format=tar --prefix="$DST_DIR/" HEAD | tar -x -C "$TMP_DIR"
rm -rf "$TMP_DIR/$DST_DIR/deps/libmediainfo/bin" \
       "$TMP_DIR/$DST_DIR/deps/libmediainfo/lib" \
       "$TMP_DIR/$DST_DIR/win32/content/7za.exe" \
       "$TMP_DIR/$DST_DIR/macx/content/7za"
tar -C "$TMP_DIR" -zcvf "../$DST_DIR.tar.gz" "$DST_DIR"
rm -rf "$TMP_DIR"
