#!/usr/bin/env bash
# Downloads the third-party binaries that the Windows and macOS packages
# bundle (7-Zip and MediaInfo) into deps/prebuilt/<platform>/, where CMake
# picks them up. Versions and SHA-256 checksums are pinned below.
#
# usage: scripts/fetch_deps.sh [windows|macos]   (default: the current OS)
# Windows needs 7z or 7zr in PATH to unpack 7-Zip Extra; 7zr.exe is
# downloaded when neither is found.

set -euo pipefail

P7ZIP_VERSION=26.04
P7ZIP_TAG=2604
MEDIAINFO_VERSION=26.10

P7ZIP_URL=https://github.com/ip7z/7zip/releases/download/$P7ZIP_VERSION
MEDIAINFO_URL=https://mediaarea.net/download/binary/libmediainfo0/$MEDIAINFO_VERSION

expected_sha256() {
  case "$1" in
    "7z$P7ZIP_TAG-extra.7z") echo dc4b11d3399db18b063630137145f5585d8f7ac847bf3639bd1185d7d1f7cee0 ;;
    "7z$P7ZIP_TAG-mac.tar.xz") echo bee04358cbcbc7106273cee0e8d72916db2696c48067a3538c34d8cd6fd16578 ;;
    7zr.exe) echo 256feca8e274e5da655e2a284fabafd9f554365eb164862089dacd4e8276d282 ;;
    "MediaInfo_DLL_${MEDIAINFO_VERSION}_Windows_x64_WithoutInstaller.zip") echo 4efe172df014693732e8b64a004cf382b8599eaf86be26a9b38c0d2993e60e1d ;;
    "MediaInfo_DLL_${MEDIAINFO_VERSION}_Mac_x86_64+arm64.tar.bz2") echo eb50c172cce6f777383d8c69256be48c47855845a5362cea9b824fb514d5fbb8 ;;
  esac
}

platform=${1:-}
if [ -z "$platform" ]; then
  case "$(uname -s)" in
    Darwin) platform=macos ;;
    MINGW* | MSYS* | CYGWIN*) platform=windows ;;
    *) echo "Linux uses system packages (libmediainfo, 7zip/p7zip); pass windows or macos" >&2; exit 1 ;;
  esac
fi

root=$(cd "$(dirname "$0")/.." && pwd)
out="$root/deps/prebuilt/$platform"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

sha256() {
  if command -v sha256sum >/dev/null; then sha256sum "$1" | cut -d' ' -f1
  else shasum -a 256 "$1" | cut -d' ' -f1; fi
}

fetch() {
  local url=$1 name
  name=$(basename "$url")
  echo "Downloading $name"
  curl -sSfL --retry 3 -o "$work/$name" "$url"
  local actual
  actual=$(sha256 "$work/$name")
  if [ "$actual" != "$(expected_sha256 "$name")" ]; then
    echo "Checksum mismatch for $name: $actual" >&2
    exit 1
  fi
}

mkdir -p "$out"

case "$platform" in
  windows)
    fetch "$P7ZIP_URL/7z$P7ZIP_TAG-extra.7z"
    unpacker=$(command -v 7z || command -v 7zr || true)
    if [ -z "$unpacker" ]; then
      fetch "$P7ZIP_URL/7zr.exe"
      unpacker="$work/7zr.exe"
    fi
    "$unpacker" x -y "-o$work/7z" "$work/7z$P7ZIP_TAG-extra.7z" x64/7za.exe License.txt >/dev/null
    cp "$work/7z/x64/7za.exe" "$out/7za.exe"
    cp "$work/7z/License.txt" "$out/7-Zip-License.txt"

    fetch "$MEDIAINFO_URL/MediaInfo_DLL_${MEDIAINFO_VERSION}_Windows_x64_WithoutInstaller.zip"
    if command -v unzip >/dev/null; then
      unzip -oq "$work/MediaInfo_DLL_${MEDIAINFO_VERSION}_Windows_x64_WithoutInstaller.zip" \
        MediaInfo.dll Developers/License.html -d "$work/mi"
    else
      "$unpacker" x -y "-o$work/mi" "$work/MediaInfo_DLL_${MEDIAINFO_VERSION}_Windows_x64_WithoutInstaller.zip" \
        MediaInfo.dll Developers/License.html >/dev/null
    fi
    cp "$work/mi/MediaInfo.dll" "$out/MediaInfo.dll"
    cp "$work/mi/Developers/License.html" "$out/MediaInfo-License.html"
    ;;
  macos)
    fetch "$P7ZIP_URL/7z$P7ZIP_TAG-mac.tar.xz"
    tar -xJf "$work/7z$P7ZIP_TAG-mac.tar.xz" -C "$work" 7zz License.txt
    # QNapi looks for 7z or 7za next to the executable and in Resources
    cp "$work/7zz" "$out/7za"
    chmod +x "$out/7za"
    cp "$work/License.txt" "$out/7-Zip-License.txt"

    fetch "$MEDIAINFO_URL/MediaInfo_DLL_${MEDIAINFO_VERSION}_Mac_x86_64+arm64.tar.bz2"
    tar -xjf "$work/MediaInfo_DLL_${MEDIAINFO_VERSION}_Mac_x86_64+arm64.tar.bz2" -C "$work" \
      MediaInfoLib/libmediainfo.0.dylib MediaInfoLib/License.html
    cp "$work/MediaInfoLib/libmediainfo.0.dylib" "$out/libmediainfo.0.dylib"
    cp "$work/MediaInfoLib/License.html" "$out/MediaInfo-License.html"
    ;;
  *)
    echo "unknown platform: $platform" >&2
    exit 1
    ;;
esac

echo "Third-party binaries for $platform are in $out:"
ls -l "$out"
