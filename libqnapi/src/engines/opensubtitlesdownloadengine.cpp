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

#include "opensubtitlesdownloadengine.h"
#include "subtitlelanguage.h"

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QPair>
#include <QSslSocket>
#include <QStringList>
#include <QTimer>

#include <algorithm>

namespace OpenSubtitlesDownloadEngineConst {
const QString defaultApiBaseUrl = "https://api.opensubtitles.com/api/v1";
const int requestTimeoutMs = 30000;
const int logoutTimeoutMs = 3000;
const int lowQuotaWarningThreshold = 2;
const QStringList subtitleExtensions = {"srt", "sub", "txt", "ssa",
                                        "ass", "smi", "vtt", "mpl"};
}  // namespace OpenSubtitlesDownloadEngineConst

using namespace OpenSubtitlesDownloadEngineConst;

std::atomic<bool> OpenSubtitlesDownloadEngine::missingApiKeyReported(false);
std::atomic<bool> OpenSubtitlesDownloadEngine::missingSslReported(false);

SubtitleDownloadEngineMetadata OpenSubtitlesDownloadEngine::metadata =
    SubtitleDownloadEngineMetadata(
        "OpenSubtitles",
        QObject::tr("<b>www.opensubtitles.com</b> subtitles download engine"),
        just(QUrl("https://www.opensubtitles.com/en/users/sign_up")),
        just(QUrl("https://www.opensubtitles.com/en/upload")));

const QUrl OpenSubtitlesDownloadEngine::apiKeyUrl =
    QUrl("https://www.opensubtitles.com/en/consumers");

OpenSubtitlesDownloadEngine::OpenSubtitlesDownloadEngine(
    const QString& tmpPath, const EngineConfig& config,
    const QSharedPointer<const P7ZipDecoder>& /*p7zipDecoder*/,
    const QString& qnapiDisplayableVersion, const QString& /*language*/)
    : SubtitleDownloadEngine(tmpPath),
      engineConfig(config),
      userAgent(QString("QNapi v%1").arg(qnapiDisplayableVersion)),
      apiBaseUrl(defaultApiBaseUrl),
      loginFailed(false),
      fileSize(0) {}

OpenSubtitlesDownloadEngine::~OpenSubtitlesDownloadEngine() {
  cleanup();
  if (isLogged()) logout();
}

SubtitleDownloadEngineMetadata OpenSubtitlesDownloadEngine::meta() const {
  return OpenSubtitlesDownloadEngine::metadata;
}

const char* const* OpenSubtitlesDownloadEngine::enginePixmapData() const {
  return OpenSubtitlesDownloadEngine::pixmapData;
}

QString OpenSubtitlesDownloadEngine::lastError() const { return error; }

// oblicza sume kontrolna dla pliku filmowego
QString OpenSubtitlesDownloadEngine::checksum(QString filename) {
  if (filename.isEmpty()) filename = movie;

  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly)) return QString("");

  fileSize = file.size();
  quint64 hash = fileSize;
  quint64 tmp, i;

  for (tmp = 0, i = 0;
       i < 65536 / sizeof(tmp) && file.read((char*)&tmp, sizeof(tmp));
       i++, hash += tmp)
    ;
  file.seek(qMax((qint64)0, (qint64)fileSize - 65536));
  for (tmp = 0, i = 0;
       i < 65536 / sizeof(tmp) && file.read((char*)&tmp, sizeof(tmp));
       i++, hash += tmp)
    ;

  return (checkSum = QString("%1").arg(hash, 16, 16, QChar('0')));
}

// szuka napisow
bool OpenSubtitlesDownloadEngine::lookForSubtitles(QString lang) {
  error.clear();

  if (engineConfig.apiKey().isEmpty()) {
    // all engines are enabled by default, so only mention the key once
    if (!missingApiKeyReported.exchange(true)) {
      error = QObject::tr(
          "OpenSubtitles requires an API key. Register an application at "
          "opensubtitles.com (Profile > API consumers) and enter the key in "
          "the OpenSubtitles engine settings.");
    }
    return false;
  }

  if (!QSslSocket::supportsSsl()) {
    if (!missingSslReported.exchange(true)) {
      error = QObject::tr(
          "OpenSubtitles needs HTTPS, but the OpenSSL libraries were not "
          "found (Qt was built for OpenSSL %1). Install them next to the "
          "QNapi executable.")
                  .arg(QSslSocket::sslLibraryBuildVersionString());
    }
    return false;
  }

  if (checkSum.isEmpty()) return false;

  QString apiLang = toApiLanguage(SubtitleLanguage(lang).toTwoLetter());
  if (apiLang.isEmpty()) return false;

  bool hasCredentials =
      !engineConfig.nick().isEmpty() && !engineConfig.password().isEmpty();
  if (hasCredentials && !isLogged() && !loginFailed) login();

  // the API puts moviehash matches first and adds file name matches after them
  QUrlQuery query;
  query.addQueryItem("foreign_parts_only", "exclude");
  query.addQueryItem("languages", apiLang);
  query.addQueryItem("moviehash", checkSum.toLower());
  QString movieBaseName = QFileInfo(movie).completeBaseName();
  if (!movieBaseName.isEmpty())
    query.addQueryItem("query", movieBaseName.toLower());

  Response r = send(apiRequest("/subtitles", query), "GET");
  if (r.status != 200) {
    error = errorFor(r);
    return false;
  }

  QSet<qint64> seenFileIds;

  for (const QJsonValue& item : r.json.value("data").toArray()) {
    QJsonObject attributes = item.toObject().value("attributes").toObject();
    QJsonArray files = attributes.value("files").toArray();

    // QNapi handles only single-file subtitles
    if (files.size() != 1) continue;

    QJsonObject file = files.at(0).toObject();
    qint64 fileId = file.value("file_id").toVariant().toLongLong();
    if (fileId <= 0 || seenFileIds.contains(fileId)) continue;
    seenFileIds.insert(fileId);

    QString fileName = file.value("file_name").toString();
    QString format = QFileInfo(fileName).suffix().toLower();
    if (!subtitleExtensions.contains(format)) format = "srt";

    QString subtitleName = attributes.value("release").toString().trimmed();
    if (subtitleName.isEmpty()) subtitleName = movieBaseName;

    // results found only by file name may belong to another release or
    // episode; SUBTITLE_BAD keeps QNapi from downloading them without asking
    SubtitleResolution resolution =
        attributes.value("moviehash_match").toBool() ? SUBTITLE_GOOD
                                                     : SUBTITLE_BAD;

    subtitlesList << SubtitleInfo(
        fromApiLanguage(attributes.value("language").toString()), meta().name(),
        QString::number(fileId), subtitleName,
        attributes.value("comments").toString().trimmed(), format, resolution);
  }

  return !subtitlesList.isEmpty();
}

QList<SubtitleInfo> OpenSubtitlesDownloadEngine::listSubtitles() {
  std::sort(subtitlesList.begin(), subtitlesList.end());
  return subtitlesList;
}

bool OpenSubtitlesDownloadEngine::download(QUuid id) {
  error.clear();

  Maybe<SubtitleInfo> ms = resolveById(id);
  if (!ms) return false;

  SubtitleInfo s = ms.value();

  QJsonObject requestBody;
  requestBody.insert("file_id", s.sourceLocation.toLongLong());

  Response r = send(apiRequest("/download"), "POST",
                    QJsonDocument(requestBody).toJson(QJsonDocument::Compact));
  if (r.status != 200) {
    error = errorFor(r);
    return false;
  }

  QString link = r.json.value("link").toString();
  if (link.isEmpty()) {
    error = QObject::tr("OpenSubtitles did not return a download link.");
    return false;
  }

  QString format =
      QFileInfo(r.json.value("file_name").toString()).suffix().toLower();
  if (subtitleExtensions.contains(format)) s.format = format;

  QNetworkRequest fileRequest{QUrl(link)};
  fileRequest.setRawHeader("User-Agent", userAgent.toUtf8());
  Response file = send(fileRequest, "GET");
  if (file.status != 200 || file.body.isEmpty()) {
    error = file.status == 0
                ? errorFor(file)
                : QObject::tr("OpenSubtitles: subtitle file download failed "
                              "(HTTP %1).")
                      .arg(file.status);
    return false;
  }

  QJsonValue remaining = r.json.value("remaining");
  if (remaining.isDouble() && remaining.toInt() <= lowQuotaWarningThreshold) {
    error = QObject::tr("OpenSubtitles: %n download(s) left in the daily "
                        "quota. %1",
                        nullptr, qMax(0, remaining.toInt()))
                .arg(r.json.value("message").toString().trimmed());
  }

  subFileName = generateTmpFileName() + "." + s.format;
  s.sourceLocation = generateTmpPath();

  QFile out(s.sourceLocation);
  if (out.exists()) out.remove();
  if (!out.open(QIODevice::WriteOnly)) return false;

  qint64 written = out.write(file.body);
  out.close();

  updateSubtitleInfo(s);

  return written == file.body.size();
}

bool OpenSubtitlesDownloadEngine::unpack(QUuid id) {
  Maybe<SubtitleInfo> ms = resolveById(id);
  if (!ms) return false;

  if (!QFile::exists(movie)) return false;

  QString downloadedPath = ms.value().sourceLocation;
  subtitlesTmp = tmpPath + QDir::separator() + subFileName;

  if (QFile::exists(subtitlesTmp)) QFile::remove(subtitlesTmp);
  if (!QFile::copy(downloadedPath, subtitlesTmp)) return false;
  QFile::remove(downloadedPath);

  return true;
}

void OpenSubtitlesDownloadEngine::cleanup() {
  clearSubtitlesList();
  if (QFile::exists(subtitlesTmp)) QFile::remove(subtitlesTmp);
}

bool OpenSubtitlesDownloadEngine::login() {
  QJsonObject requestBody;
  requestBody.insert("username", engineConfig.nick());
  requestBody.insert("password", engineConfig.password());

  Response r = send(apiRequest("/login"), "POST",
                    QJsonDocument(requestBody).toJson(QJsonDocument::Compact));

  if (r.status == 200) {
    token = r.json.value("token").toString();
    QString baseUrl = r.json.value("base_url").toString();
    if (!baseUrl.isEmpty()) {
      if (!baseUrl.startsWith("http")) baseUrl = "https://" + baseUrl;
      apiBaseUrl = baseUrl + "/api/v1";
    }
    if (isLogged()) return true;
  }

  // the API asks clients not to retry a failed login with the same credentials
  loginFailed = true;

  if (r.status == 401 && !r.json.contains("reset_time") &&
      !r.json.value("message").toString().contains("allowed")) {
    error = QObject::tr(
        "OpenSubtitles login failed: check your username and password. "
        "Continuing without logging in.");
  } else {
    error = errorFor(r);
  }
  return false;
}

void OpenSubtitlesDownloadEngine::logout() {
  // keep shutdown fast when offline
  send(apiRequest("/logout"), "DELETE", QByteArray(), logoutTimeoutMs);
  token.clear();
}

QString OpenSubtitlesDownloadEngine::toApiLanguage(const QString& lang) {
  QString l = lang.toLower();
  if (l == "pb") return "pt-br";
  if (l == "pt") return "pt-pt";
  if (l == "zh") return "zh-cn";
  return l;
}

QString OpenSubtitlesDownloadEngine::fromApiLanguage(const QString& apiLang) {
  QString l = apiLang.toLower();
  if (l == "pt-br") return "pb";
  if (l == "pt-pt") return "pt";
  if (l.startsWith("zh")) return "zh";
  return l;
}

QNetworkRequest OpenSubtitlesDownloadEngine::apiRequest(
    const QString& path, const QUrlQuery& query) const {
  QUrl url(apiBaseUrl + path);

  // the API redirects unless parameters are sorted, lowercase and use '+'
  QList<QPair<QString, QString>> items = query.queryItems();
  std::sort(items.begin(), items.end());
  QStringList parts;
  for (const QPair<QString, QString>& item : items) {
    QByteArray value = QUrl::toPercentEncoding(item.second.toLower());
    value.replace("%20", "+");
    parts << item.first + "=" + QString::fromLatin1(value);
  }
  if (!parts.isEmpty()) url.setQuery(parts.join("&"), QUrl::StrictMode);

  QNetworkRequest req(url);
  req.setRawHeader("Api-Key", engineConfig.apiKey().toUtf8());
  req.setRawHeader("User-Agent", userAgent.toUtf8());
  req.setRawHeader("Accept", "application/json");
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  if (isLogged()) req.setRawHeader("Authorization", "Bearer " + token.toUtf8());
  return req;
}

OpenSubtitlesDownloadEngine::Response OpenSubtitlesDownloadEngine::send(
    const QNetworkRequest& request, const QByteArray& verb,
    const QByteArray& data, int timeoutMs) {
  QNetworkRequest req(request);
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
  req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#endif

  QNetworkReply* reply;
  if (verb == "POST") {
    reply = manager.post(req, data);
  } else if (verb == "DELETE") {
    reply = manager.deleteResource(req);
  } else {
    reply = manager.get(req);
  }

  bool timedOut = false;
  QTimer timer;
  timer.setSingleShot(true);
  QObject::connect(&timer, &QTimer::timeout, [&timedOut, reply]() {
    timedOut = true;
    reply->abort();
  });
  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  timer.start(timeoutMs > 0 ? timeoutMs : requestTimeoutMs);
  if (!reply->isFinished()) loop.exec();
  timer.stop();

  Response r;
  r.status = 0;
  r.timedOut = timedOut;

  if (!timedOut) {
    r.status =
        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    r.reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute)
                   .toString();
    if (reply->isOpen()) r.body = reply->readAll();
    r.json = QJsonDocument::fromJson(r.body).object();
    if (r.status == 0) r.networkError = reply->errorString();
  }

  reply->deleteLater();
  return r;
}

QString OpenSubtitlesDownloadEngine::errorFor(const Response& r) const {
  if (r.timedOut) {
    return QObject::tr(
        "OpenSubtitles did not respond in time. Check your internet "
        "connection and try again.");
  }
  if (r.status == 0) {
    return QObject::tr("OpenSubtitles: network error: %1").arg(r.networkError);
  }

  QString message = r.json.value("message").toString().trimmed();
  if (message.isEmpty() && r.json.value("errors").isArray()) {
    QStringList errors;
    for (const QJsonValue& e : r.json.value("errors").toArray())
      errors << e.toString();
    message = errors.join("; ");
  }
  // e.g. HTML error pages from a proxy or a 5xx without a JSON body
  if (message.isEmpty()) message = r.reason;
  if (message.isEmpty()) message = QObject::tr("no details");

  bool quotaExhausted = r.status == 406 || r.json.contains("reset_time") ||
                        message.contains("allowed");

  if (quotaExhausted && (r.status == 406 || r.status == 401)) {
    QString text =
        QObject::tr("OpenSubtitles download quota exhausted: %1").arg(message);
    if (!isLogged()) {
      text += " " + QObject::tr(
                        "Log in with an opensubtitles.com account in the "
                        "engine settings to raise the limit.");
    }
    return text;
  }
  if (r.status == 403 && message.contains("consume", Qt::CaseInsensitive)) {
    // this is how the API reports a missing or invalid API key
    return QObject::tr(
        "OpenSubtitles rejected the API key. Check the key in the "
        "OpenSubtitles engine settings.");
  }
  if (r.status == 403) {
    return QObject::tr("OpenSubtitles refused the request: %1").arg(message);
  }
  if (r.status == 401) {
    return QObject::tr(
        "OpenSubtitles rejected the login: check your username and password "
        "in the OpenSubtitles engine settings.");
  }
  if (r.status == 429) {
    return QObject::tr("OpenSubtitles rate limit exceeded, try again shortly.");
  }
  return QObject::tr("OpenSubtitles returned HTTP %1: %2")
      .arg(r.status)
      .arg(message);
}

const char* const OpenSubtitlesDownloadEngine::pixmapData[] = {
    "16 16 14 1",       ". c #000000",      "h c #111111",
    "c c #222222",      "j c #333333",      "g c #444444",
    "l c #555555",      "e c #777777",      "k c #888888",
    "a c #999999",      "b c #aaaaaa",      "f c #cccccc",
    "i c #dddddd",      "d c #eeeeee",      "# c #ffffff",
    "................", ".##.##.##.##.##.", ".##.##.##.##.##.",
    "................", "................", "...a##b..cd##d..",
    "..ea..be.fg.hi..", "..ic..hi.ig.....", "..dh..hd.g##ij..",
    "..ea..ak.k..gi..", "...b##b..fd##l..", "................",
    "................", ".##.##.##.##.##.", ".##.##.##.##.##.",
    "................"};
