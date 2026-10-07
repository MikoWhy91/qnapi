#include <QDir>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkProxy>
#include <QSslSocket>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QtTest>

#include "engines/opensubtitlesdownloadengine.h"
#include "mockhttpserver.h"
#include "testutils.h"

typedef MockHttpServer::Request Request;
typedef MockHttpServer::Response Response;

namespace {

QJsonObject result(qint64 fileId, const QString& fileName, bool hashMatch,
                   const QString& language = "pl",
                   const QString& release = "Release.Name") {
  QJsonObject file{{"file_id", fileId}, {"file_name", fileName}};
  QJsonObject attributes{{"language", language},
                         {"release", release},
                         {"comments", " a comment "},
                         {"moviehash_match", hashMatch},
                         {"files", QJsonArray{file}}};
  return QJsonObject{{"attributes", attributes}};
}

QByteArray toJson(const QJsonObject& o) {
  return QJsonDocument(o).toJson(QJsonDocument::Compact);
}

}  // namespace

// The OpenSubtitles engine against a local mock of the opensubtitles.com
// REST API; nothing in here talks to the real service.
class TestOpenSubtitles : public QObject {
  Q_OBJECT

  QTemporaryDir tmp;
  MockHttpServer api;
  MockHttpServer loginApi;
  QString moviePath;

  QSharedPointer<OpenSubtitlesDownloadEngine> engine(
      const EngineConfig& config = EngineConfig("", "", "test-key"),
      MockHttpServer* server = nullptr) {
    QSharedPointer<OpenSubtitlesDownloadEngine> e(
        new OpenSubtitlesDownloadEngine(tmp.path(), config,
                                        QSharedPointer<const P7ZipDecoder>(),
                                        "test", "pl"));
    e->setApiBaseUrl((server ? server : &api)->baseUrl() + "/api/v1");
    e->setRequestTimeout(5000);
    e->setMoviePath(moviePath);
    e->checksum(moviePath);
    return e;
  }

  QList<Request> requestsTo(const MockHttpServer& server, const QString& path) {
    QList<Request> matching;
    for (const Request& r : server.requests()) {
      if (r.path == path) matching << r;
    }
    return matching;
  }

  SubtitleInfo findByName(const QList<SubtitleInfo>& list,
                          const QString& name) {
    for (const SubtitleInfo& s : list) {
      if (s.name == name) return s;
    }
    return SubtitleInfo();
  }

 private slots:
  void initTestCase() {
    QVERIFY(tmp.isValid());
    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
    QVERIFY(api.listen());
    QVERIFY(loginApi.listen());
    moviePath = tmp.filePath("My Movie 2020.mkv");
    QVERIFY(TestUtils::writeFile(moviePath, TestUtils::pattern(200000)));
  }

  void init() {
    api.clearRequests();
    loginApi.clearRequests();
    api.setHandler(
        [](const Request&) { return MockHttpServer::json(404, "{}"); });
  }

  // must run first: the warning is only shown once per process
  void missingApiKeyIsReportedOnce() {
    auto first = engine(EngineConfig());
    QVERIFY(!first->lookForSubtitles("pl"));
    QVERIFY(first->lastError().contains("API key"));

    auto second = engine(EngineConfig());
    QVERIFY(!second->lookForSubtitles("pl"));
    QVERIFY(second->lastError().isEmpty());
    QVERIFY(api.requests().isEmpty());
  }

  // CI sets QNAPI_TEST_REQUIRE_OPENSSL where it ships OpenSSL (Windows) to
  // check that Qt can load those libraries
  void openSslIsAvailable() {
    if (qEnvironmentVariable("QNAPI_TEST_REQUIRE_OPENSSL") != "1") {
      QSKIP("QNAPI_TEST_REQUIRE_OPENSSL is not set");
    }
    QVERIFY(QSslSocket::supportsSsl());
    QVERIFY2(QSslSocket::sslLibraryVersionString().contains("OpenSSL"),
             qPrintable(QSslSocket::sslLibraryVersionString()));
  }

  void search() {
    api.setHandler([](const Request& r) {
      if (r.path != "/api/v1/subtitles") return MockHttpServer::json(404, "{}");
      QJsonObject twoFiles = result(333, "a.srt", true);
      QJsonObject attributes = twoFiles["attributes"].toObject();
      attributes["files"] = QJsonArray{QJsonObject{{"file_id", 333}},
                                       QJsonObject{{"file_id", 334}}};
      twoFiles["attributes"] = attributes;
      QJsonArray data{result(111, "Movie.2020.srt", true),
                      result(222, "Other.SUB", false, "pl", ""), twoFiles,
                      result(111, "Duplicate.srt", true, "pl", "Duplicate"),
                      result(444, "file.weird", true, "pt-BR", "Brazil")};
      return MockHttpServer::json(200, toJson(QJsonObject{{"data", data}}));
    });

    auto e = engine();
    QString hash = e->checksum(moviePath);
    QVERIFY(e->lookForSubtitles("pl"));
    QVERIFY(e->lastError().isEmpty());

    QCOMPARE(api.requests().size(), 1);
    const Request r = api.requests().first();
    QCOMPARE(r.method, QByteArray("GET"));
    QCOMPARE(r.path, QString("/api/v1/subtitles"));
    // sorted, lower-case parameters with '+' for spaces avoid redirects
    QCOMPARE(r.rawQuery,
             QByteArray("foreign_parts_only=exclude&languages=pl&moviehash=" +
                        hash.toLatin1() + "&query=my+movie+2020"));
    QCOMPARE(r.header("Api-Key"), QByteArray("test-key"));
    QCOMPARE(r.header("User-Agent"), QByteArray("QNapi vtest"));
    QCOMPARE(r.header("Accept"), QByteArray("application/json"));
    QVERIFY(r.header("Authorization").isEmpty());

    QList<SubtitleInfo> list = e->listSubtitles();
    QCOMPARE(list.size(), 3);

    SubtitleInfo matching = findByName(list, "Release.Name");
    QCOMPARE(matching.sourceLocation, QString("111"));
    QCOMPARE(matching.resolution, SUBTITLE_GOOD);
    QCOMPARE(matching.format, QString("srt"));
    QCOMPARE(matching.lang, QString("pl"));
    QCOMPARE(matching.comment, QString("a comment"));
    QCOMPARE(matching.engine, QString("OpenSubtitles"));

    // found by file name only: never downloaded without asking
    SubtitleInfo byName = findByName(list, "My Movie 2020");
    QCOMPARE(byName.sourceLocation, QString("222"));
    QCOMPARE(byName.resolution, SUBTITLE_BAD);
    QCOMPARE(byName.format, QString("sub"));

    SubtitleInfo brazil = findByName(list, "Brazil");
    QCOMPARE(brazil.lang, QString("pb"));
    QCOMPARE(brazil.format, QString("srt"));
  }

  void searchLanguageCodes() {
    api.setHandler([](const Request&) {
      return MockHttpServer::json(200, R"({"data": []})");
    });
    auto e = engine();
    QVERIFY(!e->lookForSubtitles("pb"));
    QVERIFY(!e->lookForSubtitles("zh"));
    QCOMPARE(api.requests().size(), 2);
    QCOMPARE(api.requests()[0].query.queryItemValue("languages"),
             QString("pt-br"));
    QCOMPARE(api.requests()[1].query.queryItemValue("languages"),
             QString("zh-cn"));
  }

  void download() {
    const QByteArray subtitles = "1\n00:00:01,000 --> 00:00:02,000\nTekst\n";
    QString fileUrl = api.baseUrl() + "/files/111";
    api.setHandler([&](const Request& r) {
      if (r.path == "/api/v1/subtitles") {
        QJsonArray data{result(111, "Movie.srt", true)};
        return MockHttpServer::json(200, toJson(QJsonObject{{"data", data}}));
      }
      if (r.path == "/api/v1/download") {
        return MockHttpServer::json(
            200, toJson(QJsonObject{{"link", fileUrl},
                                    {"file_name", "Movie.srt"},
                                    {"remaining", 10}}));
      }
      if (r.path == "/files/111") {
        Response response;
        response.body = subtitles;
        response.contentType = "application/x-subrip";
        return response;
      }
      return MockHttpServer::json(404, "{}");
    });

    auto e = engine();
    QVERIFY(e->lookForSubtitles("pl"));
    SubtitleInfo s = e->listSubtitles().first();
    QVERIFY(e->download(s.id));
    QVERIFY(e->lastError().isEmpty());

    QList<Request> downloads = requestsTo(api, "/api/v1/download");
    QCOMPARE(downloads.size(), 1);
    QCOMPARE(downloads[0].method, QByteArray("POST"));
    QCOMPARE(QJsonDocument::fromJson(downloads[0].body)
                 .object()["file_id"]
                 .toVariant()
                 .toLongLong(),
             111LL);
    QCOMPARE(downloads[0].header("Api-Key"), QByteArray("test-key"));
    QCOMPARE(requestsTo(api, "/files/111").size(), 1);

    QVERIFY(e->unpack(s.id));
    QStringList unpacked =
        QDir(tmp.path()).entryList({"QNapi.*.tmp.srt"}, QDir::Files);
    QCOMPARE(unpacked.size(), 1);
    QCOMPARE(TestUtils::readFile(tmp.filePath(unpacked.first())), subtitles);
    e->cleanup();
    QVERIFY(QDir(tmp.path()).entryList({"QNapi.*"}, QDir::Files).isEmpty());
  }

  void downloadWarnsAboutLowQuota() {
    api.setHandler([&](const Request& r) {
      if (r.path == "/api/v1/subtitles") {
        QJsonArray data{result(111, "Movie.srt", true)};
        return MockHttpServer::json(200, toJson(QJsonObject{{"data", data}}));
      }
      if (r.path == "/api/v1/download") {
        return MockHttpServer::json(
            200, toJson(QJsonObject{{"link", api.baseUrl() + "/files/1"},
                                    {"remaining", 1},
                                    {"message", "Resets in 5 hours"}}));
      }
      Response response;
      response.body = "subtitles";
      return response;
    });
    auto e = engine();
    QVERIFY(e->lookForSubtitles("pl"));
    QVERIFY(e->download(e->listSubtitles().first().id));
    QCOMPARE(e->lastError(),
             QString("OpenSubtitles: 1 download(s) left in the daily quota. "
                     "Resets in 5 hours"));
  }

  void downloadQuotaExhausted() {
    api.setHandler([](const Request& r) {
      if (r.path == "/api/v1/subtitles") {
        QJsonArray data{result(111, "Movie.srt", true)};
        return MockHttpServer::json(200, toJson(QJsonObject{{"data", data}}));
      }
      return MockHttpServer::json(
          406, R"({"message": "You have downloaded your allowed 5 subtitles",)"
               R"( "reset_time": "5 hours"})");
    });
    auto e = engine();
    QVERIFY(e->lookForSubtitles("pl"));
    QVERIFY(!e->download(e->listSubtitles().first().id));
    QVERIFY(e->lastError().startsWith(
        "OpenSubtitles download quota exhausted: You have downloaded your "
        "allowed 5 subtitles"));
    QVERIFY(e->lastError().contains("Log in"));
  }

  void loginUsesBearerTokenAndBaseUrl() {
    loginApi.setHandler([](const Request& r) {
      if (r.path == "/api/v1/subtitles") {
        return MockHttpServer::json(200, R"({"data": []})");
      }
      return MockHttpServer::json(200, "{}");
    });
    QString vipBaseUrl = loginApi.baseUrl();
    api.setHandler([&](const Request& r) {
      if (r.path == "/api/v1/login") {
        return MockHttpServer::json(
            200,
            toJson(QJsonObject{{"token", "tok123"}, {"base_url", vipBaseUrl}}));
      }
      return MockHttpServer::json(404, "{}");
    });

    {
      auto e = engine(EngineConfig("user", "pass", "test-key"));
      QVERIFY(!e->lookForSubtitles("pl"));
      QVERIFY(e->lastError().isEmpty());

      QList<Request> logins = requestsTo(api, "/api/v1/login");
      QCOMPARE(logins.size(), 1);
      QCOMPARE(logins[0].method, QByteArray("POST"));
      QJsonObject credentials =
          QJsonDocument::fromJson(logins[0].body).object();
      QCOMPARE(credentials["username"].toString(), QString("user"));
      QCOMPARE(credentials["password"].toString(), QString("pass"));
      QCOMPARE(api.requests().size(), 1);

      QList<Request> searches = requestsTo(loginApi, "/api/v1/subtitles");
      QCOMPARE(searches.size(), 1);
      QCOMPARE(searches[0].header("Authorization"),
               QByteArray("Bearer tok123"));
      QCOMPARE(searches[0].header("Api-Key"), QByteArray("test-key"));

      // logged in once per engine
      e->lookForSubtitles("en");
      QCOMPARE(requestsTo(api, "/api/v1/login").size(), 1);
    }

    QList<Request> logouts = requestsTo(loginApi, "/api/v1/logout");
    QCOMPARE(logouts.size(), 1);
    QCOMPARE(logouts[0].method, QByteArray("DELETE"));
    QCOMPARE(logouts[0].header("Authorization"), QByteArray("Bearer tok123"));
  }

  void failedLoginContinuesAnonymously() {
    api.setHandler([](const Request& r) {
      if (r.path == "/api/v1/login") {
        return MockHttpServer::json(401,
                                    R"({"message": "Invalid credentials"})");
      }
      return MockHttpServer::json(200, R"({"data": []})");
    });
    auto e = engine(EngineConfig("user", "wrong", "test-key"));
    QVERIFY(!e->lookForSubtitles("pl"));
    QVERIFY(e->lastError().contains("login failed"));
    QList<Request> searches = requestsTo(api, "/api/v1/subtitles");
    QCOMPARE(searches.size(), 1);
    QVERIFY(searches[0].header("Authorization").isEmpty());

    // a failed login is not retried with the same credentials
    e->lookForSubtitles("pl");
    QCOMPARE(requestsTo(api, "/api/v1/login").size(), 1);
  }

  void errors_data() {
    QTest::addColumn<int>("status");
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<QString>("expected");
    QTest::newRow("401") << 401 << QByteArray(R"({"message": "x"})")
                         << "OpenSubtitles rejected the login: check your "
                            "username and password in the OpenSubtitles "
                            "engine settings.";
    QTest::newRow("403 invalid API key")
        << 403
        << QByteArray(R"({"message": "You cannot consume this service"})")
        << "OpenSubtitles rejected the API key. Check the key in the "
           "OpenSubtitles engine settings.";
    QTest::newRow("403 other")
        << 403 << QByteArray(R"({"errors": ["first", "second"]})")
        << "OpenSubtitles refused the request: first; second";
    QTest::newRow("406")
        << 406 << QByteArray(R"({"message": "Download limit reached"})")
        << "OpenSubtitles download quota exhausted: Download limit reached "
           "Log in with an opensubtitles.com account in the engine settings "
           "to raise the limit.";
    QTest::newRow("429") << 429 << QByteArray(R"({"message": "Slow down"})")
                         << "OpenSubtitles rate limit exceeded, try again "
                            "shortly.";
    QTest::newRow("500 without JSON")
        << 500 << QByteArray("<html>oops</html>")
        << "OpenSubtitles returned HTTP 500: Internal Server Error";
  }

  void errors() {
    QFETCH(int, status);
    QFETCH(QByteArray, body);
    QFETCH(QString, expected);
    api.setHandler(
        [&](const Request&) { return MockHttpServer::json(status, body); });
    auto e = engine();
    QVERIFY(!e->lookForSubtitles("pl"));
    QCOMPARE(e->lastError(), expected);
  }

  void timeout() {
    api.setHandler([](const Request&) {
      Response r;
      r.hang = true;
      return r;
    });
    auto e = engine();
    e->setRequestTimeout(300);
    QElapsedTimer timer;
    timer.start();
    QVERIFY(!e->lookForSubtitles("pl"));
    QVERIFY(timer.elapsed() < 5000);
    QCOMPARE(e->lastError(),
             QString("OpenSubtitles did not respond in time. Check your "
                     "internet connection and try again."));
  }

  void connectionRefused() {
    QTcpServer probe;
    QVERIFY(probe.listen(QHostAddress::LocalHost, 0));
    const quint16 unusedPort = probe.serverPort();
    probe.close();
    auto e = engine();
    e->setApiBaseUrl(QString("http://127.0.0.1:%1/api/v1").arg(unusedPort));
    QVERIFY(!e->lookForSubtitles("pl"));
    QVERIFY(e->lastError().startsWith("OpenSubtitles: network error: "));
  }
};

QTEST_GUILESS_MAIN(TestOpenSubtitles)
#include "tst_opensubtitles.moc"
