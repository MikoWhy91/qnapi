# QNapi

QNapi is free software for automatic fetching subtitles for given movie file.
It uses online databases such as NapiProjekt, OpenSubtitles.com and Napisy24.
It is based on Qt5 library, so it can be launched on any supported operating
system, including Windows, OSX and Linux.

## Binary packages

Latest binary packages are available at http://qnapi.github.io/#download

## Building from source

#### Prerequisites

* C++ compiler with c++11 support installed (`clang++`, `g++` or *MinGW* for Windows), present in `PATH`
* Qt 5.2+ (most recent 5.x recommended) installed with `qmake` present in `PATH` (you can find one at http://www.qt.io/download-open-source/)

#### Binary prerequisites

QNapi requires these binary dependencies:

* p7zip (7z, 7za) - to unpack subtitles, which are commonly compressed with 7zip
* libmediainfo - to retrieve movie info such as dimensions, duration and frame rate

Linux/UNIX users can find these dependencies in separate packages.

Statically compiled **p7zip** binaries are provided in this repository for Windows/OSX
users at `win32/content/7za.exe` and `macx/content/7za`, respectively. Similarly,
compiled **libmediainfo** libraries are provided for Windows/OSX in
`deps/libmediainfo/`.

> **WARNING!** Precompiled binaries are stripped from the source archive!

#### Cloning the source code

First, you have to clone project source code using git client:

`$ git clone https://github.com/MikoWhy91/qnapi.git`

#### Compiling

To compile the application, you have to execute two following commands in `qnapi` root directory:

`$ qmake`

This will produce `Makefile`.

> By appending `CONFIG+=no_cli` or `CONFIG+=no_gui` to qmake invocation you can disable building
> command-line or graphical interface binaries.


`$ make` (or `mingw32-make` on Windows)

This will compile the sources and build executable binary (or app bundle on OSX).

> **Important!** Windows users have to execute one more command:
>
> `$ make install` (or `mingw32-make install`)
>
> This one will copy all binaries, libraries and other dependencies to `win32/out` directory.

#### Running

By default, output binaries are placed by `make` in different locations, depending on your operating system:

* Linux - `qnapi` in root project directory
* OSX - `macx/QNapi.app` bundle
* Windows - `win32/out/qnapi.exe` executable

After you locate your binaries, you can run the application.

> ##### 7zip note #####
> For proper subtitle extraction after download, *7zip* executable is required to be passed in application's settings.
> Linux users have to install 7zip binary package from distribution repositories or compile on its own.
> For Windows and OSX there are pre-built binaries included in this repository, in `win32` and `macx` directories appropriately and should be automatically detected by the application.

## OpenSubtitles configuration

The OpenSubtitles engine uses the [OpenSubtitles.com REST API](https://opensubtitles.stoplight.io/docs/opensubtitles-api).
The legacy opensubtitles.org XML-RPC API is no longer available for regular accounts.

The API requires a personal **API key**, which is free:

1. Create an account at https://www.opensubtitles.com.
2. Open https://www.opensubtitles.com/en/consumers (*Profile > API consumers*) and create a new consumer for QNapi.
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

The API is HTTPS-only. On Windows, Qt needs the OpenSSL DLLs next to the QNapi executable
(`libssl-1_1.dll` and `libcrypto-1_1.dll` for Qt 5.12.4 and newer, `libeay32.dll` and
`ssleay32.dll` for older Qt). `make install` copies them from the directory given in
`OPENSSL_BIN_DIR` (qmake variable or environment variable) and prints a warning if they are missing.

The command-line client reads the same settings. You can also set them directly in `qnapi.ini`
(`~/.config/qnapi.ini` on Linux):

```
[OpenSubtitles]
apiKey=your-api-key
nick=optional-username
password=optional-password
```

## Making redistributable package

### OSX

#### Prerequisites

You need `appdmg` script installed. You can found it at https://github.com/LinusU/node-appdmg

#### Building .dmg image

To build .dmg image for OSX with nice, drag&drop installer, you have to execute:

`$ make appdmg`

`QNapi-x.y.z.dmg` will appear in `macx` directory when command is completed.

### Windows

#### Prerequisites

You need to have **NSIS** 2.x installed. You can found it at http://nsis.sourceforge.net

Also, you will need to manually install NSIS plugin **nsProcess**. It can be found at
http://nsis.sourceforge.net/NsProcess_plugin

#### Building Windows installer

Installer script is placed at `win32/QNapi-setup.nsi`. You can build binary exe package using NSIS user interface (by `right mouse button -> compile NSIS script`) or from command line:

`$ C:\Path\To\makensis.exe QNapi-setup.nsi`

After a while, `QNapi-x.y.z-setup.exe` file will appear in `win32` directory.
