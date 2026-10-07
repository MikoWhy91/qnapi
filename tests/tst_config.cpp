#include <QDir>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include "config/configreader.h"
#include "config/configwriter.h"
#include "libqnapi.h"
#include "testutils.h"

class TestConfig : public QObject {
  Q_OBJECT

  QTemporaryDir tmp;

  ConfigReader reader() {
    return ConfigReader(tmp.path(), LibQNapi::staticConfig(),
                        LibQNapi::subtitleDownloadEngineRegistry());
  }

  QString iniPath(const QString& name) { return tmp.filePath(name + ".ini"); }

 private slots:
  void initTestCase() { QVERIFY(tmp.isValid()); }

  void defaults() {
    const QNapiConfig config = reader().readPortableConfig(iniPath("missing"));
    QVERIFY(config.firstrun());

    const GeneralConfig general = config.generalConfig();
    QVERIFY(!general.language().isEmpty());
    QCOMPARE(general.tmpPath(), QDir::tempPath());
    QCOMPARE(general.searchPolicy(), SP_BREAK_IF_FOUND);
    QCOMPARE(general.downloadPolicy(), DP_SHOW_LIST_IF_NEEDED);
    QCOMPARE(general.changePermissionsTo(), QString("644"));

    const PostProcessingConfig pp = config.postProcessingConfig();
    QVERIFY(pp.enabled());
    QCOMPARE(pp.encodingChangeMethod(), ECM_CHANGE);
    QCOMPARE(pp.encodingFrom(), QString("windows-1250"));
    QVERIFY(pp.encodingAutoDetectFrom());
    QCOMPARE(pp.encodingTo(), QString("UTF-8"));
    QCOMPARE(pp.subFormat(), QString("srt"));
    QCOMPARE(pp.removeLinesWords(), QStringList({"movie info", "synchro"}));

    QStringList engines;
    for (const auto& e : config.enabledEngines()) {
      QVERIFY(e.second);
      engines << e.first;
    }
    QCOMPARE(engines,
             QStringList({"NapiProjekt", "OpenSubtitles", "Napisy24"}));

    QVERIFY(config.enginesConfig()["OpenSubtitles"].apiKey().isEmpty());
  }

  void roundTrip() {
    QNapiConfig config = reader().readPortableConfig(iniPath("missing"));
    QMap<QString, EngineConfig> engines = config.enginesConfig();
    engines["OpenSubtitles"] =
        EngineConfig("nick", QString::fromUtf8("pąss"), "my-api-key");
    engines["NapiProjekt"] = EngineConfig("napi", "secret");
    config = config.setEnginesConfig(engines)
                 .setEnabledEngines({{"OpenSubtitles", true},
                                     {"NapiProjekt", false},
                                     {"Napisy24", true}})
                 .setGeneralConfig(config.generalConfig()
                                       .setLanguage("pl")
                                       .setBackupLanguage("en")
                                       .setSearchPolicy(SP_SEARCH_ALL)
                                       .setDownloadPolicy(DP_NEVER_SHOW_LIST)
                                       .setTmpPath(tmp.path()))
                 .setPostProcessingConfig(config.postProcessingConfig()
                                              .setEncodingTo("ISO-8859-2")
                                              .setSubFormat("mDVD")
                                              .setRemoveLinesWords({"ad"}))
                 .setScanConfig(config.scanConfig().setFilters({"*.avi"}))
                 .setLastOpenedDir("/movies");

    ConfigWriter("9.9.9").writePortableConfig(iniPath("roundtrip"), config);
    const QNapiConfig read = reader().readPortableConfig(iniPath("roundtrip"));

    QVERIFY(!read.firstrun());
    QCOMPARE(read.version(), QString("9.9.9"));
    QCOMPARE(read.enginesConfig()["OpenSubtitles"].nick(), QString("nick"));
    QCOMPARE(read.enginesConfig()["OpenSubtitles"].password(),
             QString::fromUtf8("pąss"));
    QCOMPARE(read.enginesConfig()["OpenSubtitles"].apiKey(),
             QString("my-api-key"));
    QCOMPARE(read.enginesConfig()["NapiProjekt"].nick(), QString("napi"));
    QCOMPARE(read.enabledEngines(), config.enabledEngines());
    QCOMPARE(read.generalConfig().language(), QString("pl"));
    QCOMPARE(read.generalConfig().backupLanguage(), QString("en"));
    QCOMPARE(read.generalConfig().searchPolicy(), SP_SEARCH_ALL);
    QCOMPARE(read.generalConfig().downloadPolicy(), DP_NEVER_SHOW_LIST);
    QCOMPARE(read.generalConfig().tmpPath(), tmp.path());
    QCOMPARE(read.postProcessingConfig().encodingTo(), QString("ISO-8859-2"));
    QCOMPARE(read.postProcessingConfig().subFormat(), QString("mDVD"));
    QCOMPARE(read.postProcessingConfig().removeLinesWords(),
             QStringList({"ad"}));
    QCOMPARE(read.scanConfig().filters(), QStringList({"*.avi"}));
    QCOMPARE(read.lastOpenedDir(), QString("/movies"));
  }

  void apiKeyIsStoredInEngineSection() {
    QSettings settings(iniPath("apikey"), QSettings::IniFormat);
    settings.setValue("OpenSubtitles/apiKey", "from-ini");
    settings.sync();
    const QNapiConfig config = reader().readPortableConfig(iniPath("apikey"));
    QCOMPARE(config.enginesConfig()["OpenSubtitles"].apiKey(),
             QString("from-ini"));
    QVERIFY(config.enginesConfig()["NapiProjekt"].apiKey().isEmpty());
  }

  void legacyRemoveWordsSetting() {
    QSettings settings(iniPath("legacy"), QSettings::IniFormat);
    settings.setValue("qnapi/remove_words", QStringList({"old", "words"}));
    settings.sync();
    QCOMPARE(reader()
                 .readPortableConfig(iniPath("legacy"))
                 .postProcessingConfig()
                 .removeLinesWords(),
             QStringList({"old", "words"}));
  }

  void malformedEngineListFallsBackToDefaults() {
    QSettings settings(iniPath("engines"), QSettings::IniFormat);
    settings.setValue(
        "qnapi/engines",
        QStringList({"NapiProjekt", "OpenSubtitles:on", "Napisy24:off"}));
    settings.sync();
    for (const auto& e :
         reader().readPortableConfig(iniPath("engines")).enabledEngines()) {
      QVERIFY(e.second);
    }
  }

  void invalidTmpPathFallsBackToSystemTemp() {
    QSettings settings(iniPath("tmp"), QSettings::IniFormat);
    settings.setValue("qnapi/tmp_path", tmp.filePath("does/not/exist"));
    settings.sync();
    QCOMPARE(
        reader().readPortableConfig(iniPath("tmp")).generalConfig().tmpPath(),
        QDir::tempPath());
  }

  void toStringRedactsSecrets() {
    QString s = EngineConfig("user", "secret", "key123").toString();
    QVERIFY(s.contains("nick: user"));
    QVERIFY(s.contains("password: ***"));
    QVERIFY(s.contains("apiKey: ***"));
    QVERIFY(!s.contains("secret"));
    QVERIFY(!s.contains("key123"));
  }
};

QTEST_GUILESS_MAIN(TestConfig)
#include "tst_config.moc"
