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

#include "downloadsubtitlesargparser.h"
#include "qnapicommand.h"

#include <QFileInfo>
#include <QProcess>
#include <QUrl>

DownloadSubtitlesArgParser::DownloadSubtitlesArgParser() {}

static QString resolveKioPath(const QString& rawArg) {
  QString arg = rawArg.trimmed();

  if (QFileInfo(arg).isFile()) {
    return arg;
  }

  if (arg.startsWith("file://")) {
    QString local = QUrl(arg).toLocalFile();
    if (QFileInfo(local).isFile()) {
      return local;
    }
  }

  // Jeśli Dolphin przekazał smb:// lub inny adres KIO, zamontuj go przez KIOFuse D-Bus
  if (arg.contains("://")) {
    QProcess dbus;
    dbus.start("dbus-send",
               QStringList() << "--session"
                             << "--print-reply=literal"
                             << "--dest=org.kde.KIOFuse"
                             << "/org/kde/KIOFuse"
                             << "org.kde.KIOFuse.VFS.mountUrl"
                             << ("string:" + arg));
    if (dbus.waitForFinished(3000)) {
      QString out = QString::fromUtf8(dbus.readAllStandardOutput()).trimmed();
      if (!out.isEmpty() && QFileInfo(out).isFile()) {
        return out;
      }
    }
  }

  return QString();
}

QVariant DownloadSubtitlesArgParser::parse(const QStringList& args,
                                           const QNapiConfig& config) const {
  QStringList filePaths;

  foreach (QString arg, args) {
    QString resolved = resolveKioPath(arg);
    if (!resolved.isEmpty()) {
      filePaths << resolved;
    }
  }

  if (filePaths.isEmpty()) {
    return QVariant::fromValue(NothingParsed());
  } else {
    return QVariant::fromValue(ParsedCommand{
        config,
        QVariant::fromValue(QNapiCommand::DownloadSubtitles{filePaths})});
  }
}
