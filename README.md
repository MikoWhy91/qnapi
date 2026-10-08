# QNapi

[![CI](https://github.com/MikoWhy91/qnapi/actions/workflows/ci.yml/badge.svg)](https://github.com/MikoWhy91/qnapi/actions/workflows/ci.yml)

QNapi is free software for automatic fetching subtitles for given movie file.
It uses online databases such as NapiProjekt, OpenSubtitles.com and Napisy24.
It is based on the Qt library (Qt 6, or Qt 5.15), so it can be launched on any
supported operating system, including Windows, macOS and Linux.

This repository is a maintained fork of QNapi. QNapi was originally created and
developed by Piotr Krzemiński; all credit for the original program goes to him and
to the contributors of the original project. The fork keeps the original GPL license
and copyright notices.

## Binary packages

Binary packages of this fork are published at https://github.com/MikoWhy91/qnapi/releases

## Reporting issues

Please report bugs and feature requests at https://github.com/MikoWhy91/qnapi/issues

## Building from source

#### Prerequisites

* C++17 compiler (`g++`, `clang++` or MSVC 2019+)
* CMake 3.16 or newer (Ninja recommended)
* Qt 6.2 or newer with the Core, Network, Widgets, Qt5Compat (Core5Compat) and
  Linguist tools modules, or Qt 5.15 (Core, Network, Widgets, Linguist tools).
  Qt 6 is used when both are installed.
* Linux: `libmediainfo` development files and `pkg-config`

On Debian/Ubuntu:

```
sudo apt install cmake ninja-build g++ pkg-config libmediainfo-dev \
    qt6-base-dev qt6-tools-dev qt6-l10n-tools libqt6core5compat6-dev
```

#### Runtime dependencies

QNapi uses two external programs/libraries:

* 7-Zip (`7z` or `7za`) - to unpack subtitles, which are commonly compressed with 7-Zip
* libmediainfo - to read the frame rate needed to convert between frame-based
  (MicroDVD) and time-based (SRT, MPL2, TMPlayer) subtitles

Linux uses the distribution's packages (`7zip` or `p7zip-full`, `libmediainfo`).

The Windows and macOS packages bundle both. They are not stored in the repository;
download them before configuring:

```
scripts/fetch_deps.sh            # Git Bash on Windows, Terminal on macOS
```

This puts pinned versions of 7-Zip and the MediaInfo library (with their licenses)
into `deps/prebuilt/<platform>/`, verifying their SHA-256 checksums. CMake warns if
they are missing; another location can be passed with `-DQNAPI_PREBUILT_DIR=...`.

#### Cloning the source code

`$ git clone https://github.com/MikoWhy91/qnapi.git`

#### Compiling

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Useful options (`-D<option>=<value>` when configuring):

* `QNAPI_BUILD_GUI=OFF` / `QNAPI_BUILD_CLI=OFF` - skip the graphical (`qnapi`) or
  command-line (`qnapic`) application
* `QNAPI_QT_MAJOR_VERSION=5` or `6` - pick the Qt version explicitly
* `BUILD_TESTING=OFF` - skip the tests
* `CMAKE_PREFIX_PATH=/path/to/Qt/6.x/<compiler>` - use a Qt that is not found automatically

#### Running the tests

```
ctest --test-dir build --output-on-failure
```

The tests use only local files and a local mock HTTP server, never the real
subtitle services. The 7-Zip test runs against the bundled 7-Zip or one found in
`PATH` (or `QNAPI_TEST_7ZIP`) and is skipped if there is none.

#### Running

The binaries are placed in `build/bin`: `qnapi` and `qnapic` on Linux,
`QNapi.app` and `qnapic` on macOS, `qnapi.exe` and `qnapic.exe` on Windows.

#### Installing

On Linux, `sudo cmake --install build` installs the binaries, the desktop file,
icons, man pages and documentation under `/usr/local` (configure with
`-DCMAKE_INSTALL_PREFIX=/usr` to change it).

On Windows, `cmake --install build` copies everything needed to run QNapi into
`win32\out` by default: both executables, the Qt libraries (via `windeployqt`), the
MSVC runtime, 7-Zip, MediaInfo and the OpenSSL DLLs (see below).

> ##### 7-Zip note #####
> On Windows QNapi uses the `7za.exe` next to its executable. On Linux and macOS it looks
> for `7z` or `7za` in `PATH`, then next to its executable (and in the app bundle's
> `Resources` on macOS). A different path can be set in the application's settings.

## OpenSubtitles configuration

The OpenSubtitles engine uses the [OpenSubtitles.com REST API](https://opensubtitles.stoplight.io/docs/opensubtitles-api/e3750fd63a100-getting-started).
The legacy opensubtitles.org XML-RPC API is no longer available for regular accounts.

The API requires a personal **API key**, which is free:

1. Create an account at https://www.opensubtitles.com.
2. Open https://www.opensubtitles.com/en/consumers (*Profile > API consumers*) and create a new consumer
   with the app name **`QNapi`**. QNapi sends the `User-Agent` header `QNapi v<version>`
   (e.g. `QNapi v0.2.4-snapshot`) with every request, and OpenSubtitles expects the User-Agent to
   name the application the API key was registered for.
   If you want to download without logging in, enable *allow anonymous downloads* for that consumer.
3. Paste the key into *Settings > Engines > OpenSubtitles > Configure > API key*.

Username and password are optional, but logging in raises your daily download limit. Searching is not limited.
Downloads per 24 hours, at the time of writing:

* without logging in: 5 per IP address,
* logged-in free account: 10-20, depending on your rank,
* VIP account: 1000.

When the limit is reached, QNapi shows the server's message, including when the quota resets.

Only subtitles whose hash matches the video file are downloaded automatically. Other results
found by file name are marked as possibly not matching and have to be picked from the list.

The API is HTTPS-only. On Windows, Qt needs the OpenSSL DLLs next to the QNapi executable:
`libssl-3-x64.dll` and `libcrypto-3-x64.dll` for Qt 6.5 and newer, `libssl-1_1-x64.dll` and
`libcrypto-1_1-x64.dll` for Qt 5.15 - 6.4 (without `-x64` for 32-bit builds). `cmake --install`
copies them from the directory given in `OPENSSL_BIN_DIR` (CMake cache variable or environment
variable); configuring prints a warning if they are missing. Qt's online installer and
`aqtinstall` provide matching builds (`tools_opensslv3_x64`).

The command-line client reads the same settings. You can also set them directly in `qnapi.ini`
(`~/.config/qnapi.ini` on Linux):

```
[OpenSubtitles]
apiKey=your-api-key
nick=optional-username
password=optional-password
```

## Making redistributable package

### macOS

You need the [appdmg](https://github.com/LinusU/node-appdmg) tool (`npm install -g appdmg`)
and the bundled dependencies (`scripts/fetch_deps.sh`). Then:

`$ cmake --build build --target appdmg`

This runs `macdeployqt` on `QNapi.app` and creates `build/QNapi.dmg` with a drag & drop installer.

### Windows

You need [NSIS](https://nsis.sourceforge.io) 3.x; no plugins are required.

After `cmake --install build` has filled `win32\out`, build the installer with

`$ makensis win32\QNapi-setup.nsi`

or by right-clicking the script and choosing *Compile NSIS script*. `QNapi-x.y.z-setup.exe`
appears in the `win32` directory (`makensis /DAPPVER=x.y.z` overrides the version).

### Linux

* AppImage: configure with `-DCMAKE_INSTALL_PREFIX=/usr`, build, then run
  `BUILD_DIR=build scripts/make_appimage.sh` (downloads linuxdeploy; set `QMAKE=qmake6`
  if needed).
* Debian/Ubuntu package: `dpkg-buildpackage -us -uc -b` (build dependencies are
  listed in `debian/control`).
* Source tarball without the prebuilt binaries: `scripts/make_src_tarball.sh`.
