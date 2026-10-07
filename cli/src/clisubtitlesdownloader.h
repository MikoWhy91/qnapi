/*****************************************************************************
** QNapi
** Copyright (C) 2008-2017 Piotr Krzemiński <pio.krzeminski@gmail.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
*****************************************************************************/

#ifndef CLISUBTITLESDOWNLOADER_H
#define CLISUBTITLESDOWNLOADER_H

#include "config/qnapiconfig.h"
#include "tr.h"
#include "utils/console.h"

#include <Maybe.h>
#include <QStringList>

class QNapi;

namespace CliSubtitlesDownloader {

Q_DECLARE_NAMESPACE_TR(CliSubtitlesDownloader)

enum ExitCode {
  EC_OK = 0,
  EC_P7ZIP_UNAVAILABLE = 2,
  EC_CANNOT_WRITE_TMP_DIR = 3,
  EC_NO_WRITE_PERMISSIONS = 5,
  EC_SUBTITLES_NOT_FOUND = 6,
  EC_COULD_NOT_DOWNLOAD = 7,
  EC_COULD_NOT_UNARCHIVE = 8,
  EC_COULD_NOT_MATCH = 9
};

int downloadSubtitlesFor(const Console& c, const QStringList& movieFilePaths,
                         const QNapiConfig& config);

// steps of downloadSubtitlesFor(), used directly by the tests
bool findSubtitles(const Console& c, const QNapiConfig& config, QNapi& napi);
Maybe<int> selectSubtitles(const Console& c, const QNapiConfig& config,
                           QNapi& napi);
int downloadForMovie(const Console& c, const QString& movieFilePath, int i,
                     int total, const QNapiConfig& config, QNapi& napi);
};  // namespace CliSubtitlesDownloader

#endif  // CLISUBTITLESDOWNLOADER_H
