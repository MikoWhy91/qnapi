#include <QTemporaryDir>
#include <QtTest>

#include "libqnapi.h"
#include "parser/backuplangargparser.h"
#include "parser/cliargparsersexecutor.h"
#include "parser/dontshowlistargparser.h"
#include "parser/downloadsubtitlesargparser.h"
#include "parser/extensionargparser.h"
#include "parser/formatargparser.h"
#include "parser/langargparser.h"
#include "parser/quietbatchargparser.h"
#include "parser/showhelpargparser.h"
#include "parser/showhelplanguagesargparser.h"
#include "parser/showlistargparser.h"
#include "qnapicommand.h"
#include "testutils.h"

// the same parsers, in the same order, as qnapic's main()
static QList<QSharedPointer<CliArgParser>> cliParsers() {
  return {QSharedPointer<CliArgParser>(new ShowHelpArgParser()),
          QSharedPointer<CliArgParser>(new ShowHelpLanguagesArgParser()),
          QSharedPointer<CliArgParser>(new QuietBatchArgParser()),
          QSharedPointer<CliArgParser>(new ShowListArgParser()),
          QSharedPointer<CliArgParser>(new DontShowListArgParser()),
          QSharedPointer<CliArgParser>(new LangArgParser()),
          QSharedPointer<CliArgParser>(new BackupLangArgParser()),
          QSharedPointer<CliArgParser>(new FormatArgParser()),
          QSharedPointer<CliArgParser>(new ExtensionArgParser()),
          QSharedPointer<CliArgParser>(new DownloadSubtitlesArgParser())};
}

class TestCliArgs : public QObject {
  Q_OBJECT

  QTemporaryDir tmp;
  QNapiConfig config;

 private slots:
  void initTestCase() {
    QVERIFY(tmp.isValid());
    config = ConfigReader(tmp.path(), LibQNapi::staticConfig(),
                          LibQNapi::subtitleDownloadEngineRegistry())
                 .readPortableConfig(tmp.filePath("none.ini"));
  }

  void extension_data() {
    QTest::addColumn<QString>("extension");
    QTest::addColumn<bool>("valid");
    QTest::newRow("srt") << "srt" << true;
    QTest::newRow("letters and digits") << "SRT2" << true;
    QTest::newRow("dot") << "s.rt" << false;
    QTest::newRow("empty") << "" << false;
    QTest::newRow("non-ASCII") << QString::fromUtf8("ąę") << false;
    QTest::newRow("trailing space") << "txt " << false;
  }

  void extension() {
    QFETCH(QString, extension);
    QFETCH(bool, valid);
    QVariant result = ExtensionArgParser().parse({"-e", extension}, config);
    QCOMPARE(result.canConvert<CliArgParser::ParseError>(), !valid);
    QCOMPARE(result.canConvert<CliArgParser::ParsedModifier>(), valid);
    if (valid) {
      QCOMPARE(result.value<CliArgParser::ParsedModifier>()
                   .refinedConfig.postProcessingConfig()
                   .subExtension(),
               extension);
    }
  }

  void language() {
    QVariant ok = LangArgParser().parse({"--lang", "pl"}, config);
    QVERIFY(ok.canConvert<CliArgParser::ParsedModifier>());
    QCOMPARE(ok.value<CliArgParser::ParsedModifier>()
                 .refinedConfig.generalConfig()
                 .language(),
             QString("pl"));
    QVERIFY(LangArgParser()
                .parse({"-l", "xx"}, config)
                .canConvert<CliArgParser::ParseError>());
    QVERIFY(LangArgParser()
                .parse({"-l"}, config)
                .canConvert<CliArgParser::ParseError>());
    QVERIFY(LangArgParser()
                .parse({"movie.avi"}, config)
                .canConvert<CliArgParser::NothingParsed>());
  }

  void downloadCommand() {
    QString movie = tmp.filePath("movie.avi");
    QVERIFY(TestUtils::writeFile(movie, "x"));
    auto result = CliArgParsersExecutor::executeParsers(
        cliParsers(),
        {"-q", "-l", "pl", "-lb", "en", "-f", "srt", "-e", "txt", movie,
         "missing.avi"},
        config);
    QVERIFY(result.is<Maybe<CliArgParser::ParsedCommand>>());
    auto command = result.as<Maybe<CliArgParser::ParsedCommand>>();
    QVERIFY(command);
    QVERIFY(
        command.value().command.canConvert<QNapiCommand::DownloadSubtitles>());
    QVERIFY(!command.value().command.canConvert<QNapiCommand::ShowHelp>());
    QCOMPARE(command.value()
                 .command.value<QNapiCommand::DownloadSubtitles>()
                 .movieFilePaths,
             QStringList({movie}));

    const QNapiConfig refined = command.value().refinedConfig;
    QVERIFY(refined.generalConfig().quietBatch());
    QCOMPARE(refined.generalConfig().language(), QString("pl"));
    QCOMPARE(refined.generalConfig().backupLanguage(), QString("en"));
    QCOMPARE(refined.postProcessingConfig().subFormat(), QString("srt"));
    QCOMPARE(refined.postProcessingConfig().subExtension(), QString("txt"));
  }

  void helpCommandWins() {
    auto result =
        CliArgParsersExecutor::executeParsers(cliParsers(), {"-h"}, config);
    auto command = result.as<Maybe<CliArgParser::ParsedCommand>>();
    QVERIFY(command);
    QVERIFY(command.value().command.canConvert<QNapiCommand::ShowHelp>());
  }

  void noCommand() {
    auto result =
        CliArgParsersExecutor::executeParsers(cliParsers(), {"-q"}, config);
    QVERIFY(result.is<Maybe<CliArgParser::ParsedCommand>>());
    QVERIFY(!result.as<Maybe<CliArgParser::ParsedCommand>>());
  }

  void parseError() {
    auto result = CliArgParsersExecutor::executeParsers(cliParsers(),
                                                        {"-e", "a.b"}, config);
    QVERIFY(result.is<QString>());
    QVERIFY(result.as<QString>().contains("a.b"));
  }
};

QTEST_GUILESS_MAIN(TestCliArgs)
#include "tst_cliargs.moc"
