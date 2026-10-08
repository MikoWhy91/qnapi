# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

This is the independently maintained fork at
[github.com/MikoWhy91/qnapi](https://github.com/MikoWhy91/qnapi).
Upstream QNapi last released **0.2.3** in May 2017; this skip of 0.2.4 avoids
clashing with upstream's moving `0.2.4-snapshot` builds.

## [Unreleased]

## [0.3.0] - 2026-10-08

First release of the maintained fork. A free
[OpenSubtitles.com](https://www.opensubtitles.com) API key is required for that
engine (create a consumer named **QNapi** under Profile → API consumers).

### Added

- OpenSubtitles.com REST API engine (replaces the disabled opensubtitles.org
  XML-RPC API): API key field, optional login for a higher download quota,
  two-letter language codes, and explicit handling of invalid keys, HTTP errors,
  rate limits and exhausted quota (including the server's reset time).
- Qt Test suite run through CTest (hashes, subtitle conversion, encoding,
  config, CLI arguments, 7-Zip, search/selection with fake engines, and
  OpenSubtitles against a local mock HTTP server). Tests never call live
  subtitle services.
- GitHub Actions CI for Linux (Ubuntu 22.04 / Qt 6 and Ubuntu 24.04 / Qt 5.15),
  macOS 14 / Qt 6.8, and Windows 2022 / Qt 6.8 (MSVC), including package
  artifacts.
- CMake cache variable `QNAPI_DISPLAYABLE_VERSION` so prerelease tags such as
  `0.3.0-rc1` can be shown in About, `--help` / `--version` and package names
  without changing the numeric `QNAPI_VERSION`.

### Changed

- Rebranded project links, issues and binary packages to
  https://github.com/MikoWhy91/qnapi. Original authorship remains credited to
  Piotr Krzemiński and the upstream QNapi contributors.
- Ported to Qt 6 (6.2 or newer; 6.8 in CI). Qt 5.15 remains supported.
- Replaced qmake with CMake 3.16+.
- Packaging: Debian package depends on `qt6-qpa-plugins`; NSIS installer is
  Unicode and no longer needs the nsProcess plugin; AppImage is built with
  linuxdeploy; Windows and macOS 7-Zip and MediaInfo binaries are downloaded
  by a pinned `scripts/fetch_deps.sh` instead of being committed.
- Standalone command-line binary `qnapic` (upstream #120, previously landed on
  the 0.2.4-snapshot branch).
- Subtitles whose hash does not match the video are marked as possibly not
  matching and are never auto-downloaded (including when there is only one
  result). Search continues to other engines and to the backup language.
  Possibly-not-matching results are still offered in the list at the end.
  CLI quiet/`-d` mode skips those files instead of downloading them.
- CLI exit code **6** (`EC_SUBTITLES_NOT_FOUND`) for files skipped because
  nothing matched the hash, and a non-zero status for a failed download (plain
  "not found" previously exited 0).

### Fixed

- Encoding auto-detect calling the wrong conversion overload, and leftover
  UTF-8 BOM when converting UTF-8+BOM to UTF-8 (upstream #200).
- Memory-safety issues found with ASan: dangling post-processing config
  reference (stack-use-after-scope) and uninitialized SRT timestamps.
- OpenSubtitles.org and Napisy24 searches for files larger than 2 GiB
  (upstream #178).
- Use of system directory separators (upstream #187).
- Backup language being forced to English (upstream, 0.2.4-snapshot).
- `remove_words_enabled` applied consistently (upstream #135).
- Tray icon uses `qnapi-panel` when available (upstream #131 / #138).
- Scrollbar in the About dialog when needed (upstream #139).
- Missing include for Qt 5.11 (upstream #148; superseded by the Qt 6 port).

### Packaging notes

- Windows packages ship OpenSSL 3 DLLs (`libssl-3-x64.dll`,
  `libcrypto-3-x64.dll`) and `tls/qopensslbackend.dll` so the HTTPS-only
  OpenSubtitles engine can connect.
- Linux `.deb` needs `qt6-qpa-plugins` at runtime for the xcb platform plugin
  and TLS backends.

## [0.2.3] - 2017-05-19

Last upstream release ([QNapi/qnapi](https://github.com/QNapi/qnapi)).
Earlier history is in [`doc/ChangeLog`](doc/ChangeLog) (Polish).

### Added

- UI language selection (English / Polish / Italian).
- Split into `libqnapi` and a GUI.
- CLI flags for target subtitle format and extension (#65).
- Linux x86_64 AppImage.

### Changed

- Rebuilt application configuration subsystem.

### Fixed

- Loss of colour information during subtitle conversion (#100).

[Unreleased]: https://github.com/MikoWhy91/qnapi/compare/0.3.0...HEAD
[0.3.0]: https://github.com/MikoWhy91/qnapi/compare/0.2.3...0.3.0
[0.2.3]: https://github.com/QNapi/qnapi/releases/tag/0.2.3
