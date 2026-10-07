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

#ifndef OPENSUBTITLESDOWNLOADENGINE_H
#define OPENSUBTITLESDOWNLOADENGINE_H

#include "config/engineconfig.h"
#include "engines/subtitledownloadengine.h"
#include "utils/p7zipdecoder.h"

#include <QByteArray>
#include <QEventLoop>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QSet>
#include <QUrlQuery>

class OpenSubtitlesDownloadEngine : public SubtitleDownloadEngine {
 public:
  OpenSubtitlesDownloadEngine(
      const QString& tmpPath, const EngineConfig& config,
      const QSharedPointer<const P7ZipDecoder>& p7zipDecoder,
      const QString& qnapiDisplayableVersion, const QString& language);
  ~OpenSubtitlesDownloadEngine();

  static SubtitleDownloadEngineMetadata metadata;
  static const char* const pixmapData[];
  static const QUrl apiKeyUrl;

  SubtitleDownloadEngineMetadata meta() const;
  const char* const* enginePixmapData() const;

  QString checksum(QString filename = "");
  bool lookForSubtitles(QString lang);
  QList<SubtitleInfo> listSubtitles();
  bool download(QUuid id);
  bool unpack(QUuid id);
  void cleanup();
  QString lastError() const;

  static QString toApiLanguage(const QString& lang);
  static QString fromApiLanguage(const QString& apiLang);

 private:
  struct Response {
    int status;
    QJsonObject json;
    QByteArray body;
    QString networkError;
  };

  EngineConfig engineConfig;
  QString userAgent;
  QString apiBaseUrl;
  QString token;
  bool loginFailed;
  QString error;

  quint64 fileSize;
  QString subFileName;

  QNetworkAccessManager manager;
  QEventLoop loop;

  bool isLogged() const { return !token.isEmpty(); }
  bool login();
  void logout();

  bool search(const QUrlQuery& query, QSet<qint64>* seenFileIds,
              bool* anyHashMatch);
  SubtitleResolution resolution(const QJsonObject& attributes,
                                const QString& fileName) const;

  QNetworkRequest apiRequest(const QString& path,
                             const QUrlQuery& query = QUrlQuery()) const;
  Response send(const QNetworkRequest& req, const QByteArray& verb,
                const QByteArray& data = QByteArray());
  QString errorFor(const Response& r) const;
};

#endif
