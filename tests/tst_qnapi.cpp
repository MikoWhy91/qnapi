#include <QTemporaryDir>
#include <QtTest>

#include "clisubtitlesdownloader.h"
#include "libqnapi.h"
#include "qnapi.h"
#include "testutils.h"

using namespace CliSubtitlesDownloader;
using TestUtils::FakeEngine;
typedef FakeEngine::Result R;
typedef QList<QSharedPointer<SubtitleDownloadEngine>> Engines;

Q_DECLARE_METATYPE(QList<FakeEngine::Result>)

class TestQNapi : public QObject {
  Q_OBJECT

  QTemporaryDir tmp;
  QNapiConfig baseConfig;
  const Console console{true};

  QNapiConfig config(DownloadPolicy dp = DP_SHOW_LIST_IF_NEEDED,
                     SearchPolicy sp = SP_BREAK_IF_FOUND,
                     const QString& backup = "en") {
    return baseConfig
        .setGeneralConfig(baseConfig.generalConfig()
                              .setLanguage("pl")
                              .setBackupLanguage(backup)
                              .setDownloadPolicy(dp)
                              .setSearchPolicy(sp)
                              .setQuietBatch(false)
                              .setTmpPath(tmp.path()))
        .setPostProcessingConfig(
            baseConfig.postProcessingConfig().setEnabled(false));
  }

  QSharedPointer<FakeEngine> engine(const QString& name) {
    return QSharedPointer<FakeEngine>(new FakeEngine(name, tmp.path()));
  }

  QString movie(const QString& name = "movie.avi") {
    QString path = tmp.filePath(name);
    TestUtils::writeFile(path, "movie");
    return path;
  }

 private slots:
  void initTestCase() {
    QVERIFY(tmp.isValid());
    baseConfig = ConfigReader(tmp.path(), LibQNapi::staticConfig(),
                              LibQNapi::subtitleDownloadEngineRegistry())
                     .readPortableConfig(tmp.filePath("none.ini"));
  }

  void selection_data() {
    QTest::addColumn<QList<R>>("results");
    QTest::addColumn<int>("downloadPolicy");
    QTest::addColumn<bool>("showList");
    QTest::addColumn<int>("bestIdx");

    const int ifNeeded = DP_SHOW_LIST_IF_NEEDED;
    QTest::newRow("nothing") << QList<R>{} << ifNeeded << false << -1;
    QTest::newRow("single unknown")
        << QList<R>{{"a", SUBTITLE_UNKNOWN}} << ifNeeded << false << 0;
    QTest::newRow("single good")
        << QList<R>{{"a", SUBTITLE_GOOD}} << ifNeeded << false << 0;
    QTest::newRow("single bad is never picked automatically")
        << QList<R>{{"a", SUBTITLE_BAD}} << ifNeeded << true << -1;
    QTest::newRow("only bad")
        << QList<R>{{"a", SUBTITLE_BAD}, {"b", SUBTITLE_BAD}} << ifNeeded
        << true << -1;
    QTest::newRow("two unknown need a choice")
        << QList<R>{{"a", SUBTITLE_UNKNOWN}, {"b", SUBTITLE_UNKNOWN}}
        << ifNeeded << true << 0;
    QTest::newRow("good wins over unknown")
        << QList<R>{{"a", SUBTITLE_UNKNOWN}, {"b", SUBTITLE_GOOD}} << ifNeeded
        << false << 1;
    QTest::newRow("bad results do not count as alternatives")
        << QList<R>{{"a", SUBTITLE_BAD}, {"b", SUBTITLE_UNKNOWN}} << ifNeeded
        << false << 1;
    QTest::newRow("first good after bad") << QList<R>{{"a", SUBTITLE_BAD},
                                                      {"b", SUBTITLE_GOOD},
                                                      {"c", SUBTITLE_GOOD}}
                                          << ifNeeded << false << 1;
    QTest::newRow("always show list") << QList<R>{{"a", SUBTITLE_GOOD}}
                                      << int(DP_ALWAYS_SHOW_LIST) << true << 0;
    QTest::newRow("never show list skips bad")
        << QList<R>{{"a", SUBTITLE_BAD}} << int(DP_NEVER_SHOW_LIST) << false
        << -1;
    QTest::newRow("never show list picks unknown")
        << QList<R>{{"a", SUBTITLE_UNKNOWN}, {"b", SUBTITLE_UNKNOWN}}
        << int(DP_NEVER_SHOW_LIST) << false << 0;
  }

  void selection() {
    QFETCH(QList<R>, results);
    QFETCH(int, downloadPolicy);
    QFETCH(bool, showList);
    QFETCH(int, bestIdx);

    auto e = engine("A");
    e->results["pl"] = results;
    QNapi napi(config(DownloadPolicy(downloadPolicy)), Engines{e});
    napi.setMoviePath(movie());
    napi.lookForSubtitles("pl");

    QCOMPARE(napi.needToShowList(), showList);
    QCOMPARE(napi.bestIdx(), bestIdx);
  }

  void listIsSortedByLanguagePreference() {
    auto a = engine("A");
    auto b = engine("B");
    a->results["en"] = {{"backup", SUBTITLE_GOOD}};
    a->results["de"] = {{"other", SUBTITLE_GOOD}};
    b->results["pl"] = {{"main", SUBTITLE_UNKNOWN}};
    QNapi napi(config(), Engines{a, b});
    napi.setMoviePath(movie());
    napi.lookForSubtitles("de");
    napi.lookForSubtitles("en");
    napi.lookForSubtitles("pl");

    QStringList names;
    for (const SubtitleInfo& s : napi.listSubtitles()) names << s.name;
    QCOMPARE(names, QStringList({"main", "backup", "other"}));
    QVERIFY(!napi.needToShowList());
    QCOMPARE(napi.bestIdx(), 1);
  }

  void badResultsAreNotFound() {
    auto e = engine("A");
    e->results["pl"] = {{"a", SUBTITLE_BAD}};
    QNapi napi(config(), Engines{e});
    napi.setMoviePath(movie());
    QVERIFY(!napi.lookForSubtitles("pl"));
    QCOMPARE(napi.error(), QString("No subtitles found!"));
    QVERIFY(napi.hasAnySubtitles());

    e->results["en"] = {{"b", SUBTITLE_UNKNOWN}};
    QVERIFY(napi.lookForSubtitles("en", "A"));
  }

  void engineErrorsAreCollectedOnce() {
    auto a = engine("A");
    auto b = engine("B");
    a->errorOnSearch = "A is broken";
    b->errorOnSearch = "A is broken";
    QNapi napi(config(), Engines{a, b});
    napi.setMoviePath(movie());
    napi.lookForSubtitles("pl");
    napi.lookForSubtitles("en");
    QCOMPARE(napi.takeEngineErrors(), QStringList({"A is broken"}));
    QVERIFY(napi.takeEngineErrors().isEmpty());
  }

  void downloadRejectsInvalidIndexes() {
    auto e = engine("A");
    e->results["pl"] = {{"a", SUBTITLE_GOOD}};
    QNapi napi(config(), Engines{e});
    napi.setMoviePath(movie());
    napi.lookForSubtitles("pl");
    napi.listSubtitles();
    QVERIFY(!napi.download(-1));
    QVERIFY(!napi.download(1));
    QVERIFY(!napi.unpack(-1));
    QVERIFY(e->downloadedNames.isEmpty());
    QVERIFY(napi.download(0));
    QCOMPARE(e->downloadedNames, QStringList({"a"}));
  }

  void messagesAfterSuccessfulDownloadAreNotices() {
    auto e = engine("A");
    e->results["pl"] = {{"a", SUBTITLE_GOOD}};
    QNapi napi(config(), Engines{e});
    napi.setMoviePath(movie());
    napi.lookForSubtitles("pl");
    napi.listSubtitles();
    e->errorOnSearch = "1 download left";
    QVERIFY(napi.download(0));
    QCOMPARE(napi.takeEngineNotices(), QStringList({"1 download left"}));
    QVERIFY(napi.takeEngineErrors().isEmpty());
  }

  void searchStopsAtFirstEngineWithResults() {
    auto a = engine("A");
    auto b = engine("B");
    auto c = engine("C");
    b->results["pl"] = {{"b", SUBTITLE_UNKNOWN}};
    c->results["pl"] = {{"c", SUBTITLE_GOOD}};
    QNapi napi(config(), Engines{a, b, c});
    napi.setMoviePath(movie());
    QVERIFY(findSubtitles(console, config(), napi));
    QCOMPARE(a->searchedLanguages, QStringList({"pl"}));
    QCOMPARE(b->searchedLanguages, QStringList({"pl"}));
    QVERIFY(c->searchedLanguages.isEmpty());
  }

  void searchFallsBackToBackupLanguage() {
    auto a = engine("A");
    auto b = engine("B");
    // results that may not match do not stop the search
    a->results["pl"] = {{"bad", SUBTITLE_BAD}};
    a->results["en"] = {{"en", SUBTITLE_UNKNOWN}};
    b->results["en"] = {{"en2", SUBTITLE_GOOD}};
    QNapi napi(config(), Engines{a, b});
    napi.setMoviePath(movie());
    QVERIFY(findSubtitles(console, config(), napi));
    QCOMPARE(a->searchedLanguages, QStringList({"pl", "en"}));
    QCOMPARE(b->searchedLanguages, QStringList({"pl"}));
  }

  void searchWithoutBackupLanguage() {
    auto a = engine("A");
    a->results["en"] = {{"en", SUBTITLE_GOOD}};
    QNapi napi(config(DP_SHOW_LIST_IF_NEEDED, SP_BREAK_IF_FOUND, ""),
               Engines{a});
    napi.setMoviePath(movie());
    QVERIFY(!findSubtitles(
        console, config(DP_SHOW_LIST_IF_NEEDED, SP_BREAK_IF_FOUND, ""), napi));
    QCOMPARE(a->searchedLanguages, QStringList({"pl"}));
  }

  void searchAllEngines() {
    auto a = engine("A");
    auto b = engine("B");
    a->results["pl"] = {{"a", SUBTITLE_GOOD}};
    QNapiConfig cfg = config(DP_SHOW_LIST_IF_NEEDED, SP_SEARCH_ALL);
    QNapi napi(cfg, Engines{a, b});
    napi.setMoviePath(movie());
    QVERIFY(findSubtitles(console, cfg, napi));
    QCOMPARE(a->searchedLanguages, QStringList({"pl"}));
    QCOMPARE(b->searchedLanguages, QStringList({"pl"}));
  }

  void searchAllEnginesWithBackupLanguage() {
    auto a = engine("A");
    auto b = engine("B");
    a->results["pl"] = {{"a", SUBTITLE_GOOD}};
    QNapiConfig cfg =
        config(DP_SHOW_LIST_IF_NEEDED, SP_SEARCH_ALL_WITH_BACKUP_LANG);
    QNapi napi(cfg, Engines{a, b});
    napi.setMoviePath(movie());
    QVERIFY(findSubtitles(console, cfg, napi));
    QCOMPARE(a->searchedLanguages, QStringList({"pl", "en"}));
    QCOMPARE(b->searchedLanguages, QStringList({"pl", "en"}));
  }

  void cliDownloadsBestMatch() {
    auto e = engine("A");
    e->results["pl"] = {{"unknown", SUBTITLE_UNKNOWN}, {"good", SUBTITLE_GOOD}};
    QNapi napi(config(), Engines{e});
    QString moviePath = movie("cli-ok.avi");
    QCOMPARE(downloadForMovie(console, moviePath, 1, 1, config(), napi),
             int(EC_OK));
    QCOMPARE(e->downloadedNames, QStringList({"good"}));
    QVERIFY(TestUtils::readFile(tmp.filePath("cli-ok.srt")).contains("good"));
  }

  void cliExitCodeWhenNothingFound() {
    auto e = engine("A");
    QNapi napi(config(), Engines{e});
    QCOMPARE(downloadForMovie(console, movie(), 1, 1, config(), napi),
             int(EC_SUBTITLES_NOT_FOUND));
  }

  void cliNeverDownloadsBadResultsWithoutAsking() {
    QNapiConfig cfg = config(DP_NEVER_SHOW_LIST);
    auto e = engine("A");
    e->results["pl"] = {{"bad", SUBTITLE_BAD}};
    QNapi napi(cfg, Engines{e});
    QCOMPARE(downloadForMovie(console, movie("cli-bad.avi"), 1, 1, cfg, napi),
             int(EC_SUBTITLES_NOT_FOUND));
    QVERIFY(e->downloadedNames.isEmpty());
    QVERIFY(!QFile::exists(tmp.filePath("cli-bad.srt")));

    // quiet batch mode behaves like "never show the list"
    QNapiConfig quiet =
        config().setGeneralConfig(config().generalConfig().setQuietBatch(true));
    QNapi napi2(quiet, Engines{e});
    QCOMPARE(
        downloadForMovie(console, movie("cli-bad.avi"), 1, 1, quiet, napi2),
        int(EC_SUBTITLES_NOT_FOUND));
    QVERIFY(e->downloadedNames.isEmpty());
  }

  void cliExitCodeWhenDownloadFails() {
    auto e = engine("A");
    e->results["pl"] = {{"good", SUBTITLE_GOOD}};
    e->downloadSucceeds = false;
    QNapi napi(config(), Engines{e});
    QCOMPARE(downloadForMovie(console, movie(), 1, 1, config(), napi),
             int(EC_COULD_NOT_DOWNLOAD));
  }

  void cliExitCodeWithoutWritePermission() {
#ifdef Q_OS_WIN
    QSKIP("directory permissions work differently on Windows");
#else
    QString dir = tmp.filePath("readonly");
    QDir().mkpath(dir);
    QString moviePath = dir + "/movie.avi";
    TestUtils::writeFile(moviePath, "movie");
    QFile::setPermissions(dir, QFile::ReadOwner | QFile::ExeOwner);
    if (QFileInfo(dir).isWritable()) {
      QFile::setPermissions(
          dir, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
      QSKIP("running with permissions that ignore the read-only directory");
    }
    auto e = engine("A");
    QNapi napi(config(), Engines{e});
    int result = downloadForMovie(console, moviePath, 1, 1, config(), napi);
    QFile::setPermissions(
        dir, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    QCOMPARE(result, int(EC_NO_WRITE_PERMISSIONS));
#endif
  }
};

QTEST_GUILESS_MAIN(TestQNapi)
#include "tst_qnapi.moc"
