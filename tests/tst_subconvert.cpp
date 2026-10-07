#include <QTemporaryDir>
#include <QtTest>

#include "libqnapi.h"
#include "subconvert/subtitleconverter.h"
#include "subconvert/subtitleformatsregistry.h"
#include "testutils.h"

using TestUtils::readFile;
using TestUtils::writeFile;

namespace {

const QByteArray microDvdSample =
    "{25}{50}Witaj|\xc5\x9bwiecie\r\n{75}{100}{y:i}Drugi\r\n";
const QByteArray subRipSample =
    "1\r\n00:00:01,000 --> 00:00:02,000\r\n"
    "Za\xc5\xbc\xc3\xb3\xc5\x82\xc4\x87\r\n\r\n";
const QByteArray mpl2Sample = "[10][20]Pierwszy\n[30][40]B\n";
const QByteArray tmPlayerSample = "00:00:01:Raz|Dwa\n00:00:09:Trzy\n";

QString tokens(const QVector<SubToken>& toks) {
  QStringList l;
  for (const SubToken& t : toks) {
    l << QString("%1:%2").arg(int(t.type)).arg(t.payload);
  }
  return l.join(" ");
}

}  // namespace

class TestSubConvert : public QObject {
  Q_OBJECT

  QTemporaryDir tmp;
  QSharedPointer<const SubtitleFormatsRegistry> registry =
      LibQNapi::subtitleFormatsRegistry();

  QSharedPointer<const SubtitleConverter> converter(bool skipAds,
                                                    double fps = 25.0) {
    return QSharedPointer<const SubtitleConverter>(
        new SubtitleConverter(registry,
                              QSharedPointer<const MovieInfoProvider>(
                                  new TestUtils::FixedMovieInfoProvider(fps)),
                              skipAds));
  }

  // the converter writes in text mode, i.e. with CRLF on Windows
  QString convert(const QByteArray& input, const QString& targetFormat,
                  bool skipAds = true, double fps = 25.0, bool* ok = nullptr) {
    QString in = tmp.filePath("in.txt");
    QString out = tmp.filePath("out.txt");
    writeFile(in, input);
    QFile::remove(out);
    bool result = converter(skipAds, fps)
                      ->convertSubtitles(in, targetFormat, out,
                                         tmp.filePath("movie.avi"));
    if (ok) *ok = result;
    return QString::fromUtf8(readFile(out)).replace("\r\n", "\n");
  }

 private slots:
  void initTestCase() { QVERIFY(tmp.isValid()); }

  void registeredFormats() {
    QStringList names = registry->listFormatNames();
    names.sort();
    QCOMPARE(names, QStringList({"MPL2", "SRT", "TMP", "mDVD"}));
    QCOMPARE(registry->select("srt")->defaultExtension(), QString("srt"));
    QVERIFY(registry->select("tmp")->isTimeBased());
    QVERIFY(!registry->select("mdvd")->isTimeBased());
    QVERIFY(registry->select("unknown").isNull());
  }

  void detectFormat_data() {
    QTest::addColumn<QStringList>("lines");
    QTest::addColumn<QString>("format");
    QTest::newRow("MicroDVD")
        << QStringList{"", " ", "{1}{25}Hello|World"} << "mDVD";
    QTest::newRow("MPL2") << QStringList{"[10][25]Hello"} << "MPL2";
    QTest::newRow("TMPlayer") << QStringList{"00:00:01:Hello"} << "TMP";
    QTest::newRow("SubRip")
        << QStringList{"", "1", "00:00:01,000 --> 00:00:02,500", "Line", ""}
        << "SRT";
    QTest::newRow("SubRip, number and time on one line")
        << QStringList{"1 00:00:01,000 --> 00:00:02,500", "inline", ""}
        << "SRT";
    QTest::newRow("SubRip without number")
        << QStringList{"00:00:01,000 --> 00:00:02,000", "x"} << "";
    QTest::newRow("TMPlayer needs two-digit hours")
        << QStringList{"0:00:01:bad"} << "";
    QTest::newRow("plain text") << QStringList{"junk"} << "";
  }

  void detectFormat() {
    QFETCH(QStringList, lines);
    QFETCH(QString, format);
    QCOMPARE(converter(true)->detectFormat(lines), format);
  }

  void detectFormatOfFile() {
    QString path = tmp.filePath("detect.txt");
    QVERIFY(writeFile(path, subRipSample));
    QCOMPARE(converter(true)->detectFormat(path), QString("SRT"));
    QVERIFY(writeFile(path, microDvdSample));
    QCOMPARE(converter(true)->detectFormat(path), QString("mDVD"));
  }

  void decodeMicroDvd() {
    SubFile sf = registry->select("mDVD")->decode(
        {"", "{100}{200}{y:i}Ital{/y:i} plain|/second", "not a subtitle"});
    QCOMPARE(sf.entries.size(), 1);
    QCOMPARE(sf.entries[0].frameStart, 100L);
    QCOMPARE(sf.entries[0].frameStop, 200L);
    QCOMPARE(tokens(sf.entries[0].tokens),
             QString("5: 1:Ital 6: 0: 1:plain 2: 5: 1:second"));
  }

  void decodeMpl2UsesDeciseconds() {
    SubFile sf = registry->select("MPL2")->decode({"[10][25]Hello"});
    QCOMPARE(sf.entries.size(), 1);
    QCOMPARE(sf.entries[0].frameStart, 1000L);
    QCOMPARE(sf.entries[0].frameStop, 2500L);
  }

  void decodeTmPlayerDerivesStopTimes() {
    SubFile sf = registry->select("TMP")->decode(
        {"00:00:01:Hello|World", "00:00:03:Next", "00:00:20:Last"});
    QCOMPARE(sf.entries.size(), 3);
    QCOMPARE(sf.entries[0].frameStart, 1000L);
    QCOMPARE(sf.entries[0].frameStop, 2999L);
    QCOMPARE(sf.entries[1].frameStop, 8000L);
    QCOMPARE(sf.entries[2].frameStop, 25000L);
  }

  void decodeSubRip() {
    SubFile sf = registry->select("SRT")->decode(
        {"1", "00:00:01,000 --> 00:00:02,500", "Line one", "<i>Line two</i>",
         "", "2", "00:00:03,000 --> 00:00:04,000",
         "<font color=\"#ff0000\">Red</font> and <b>b</b>", "123", "", "3",
         "00:01:03,000 --> 01:00:04,001 X1:0", "trail"});
    QCOMPARE(sf.entries.size(), 3);
    QCOMPARE(sf.entries[0].frameStart, 1000L);
    QCOMPARE(sf.entries[0].frameStop, 2500L);
    QCOMPARE(tokens(sf.entries[0].tokens),
             QString("1:Line 0: 1:one 2: 5: 1:Line 0: 1:two 6:"));
    // the greedy colour pattern runs to the last '>', and a line with only
    // a number is dropped when no text follows it
    QCOMPARE(tokens(sf.entries[1].tokens),
             QString("9:ff0000>Red</font> and <b>b</b"));
    QCOMPARE(tokens(sf.entries[2].tokens), QString("1:trail"));
    QCOMPARE(sf.entries[2].frameStart, 63000L);
    QCOMPARE(sf.entries[2].frameStop, 3604001L);
  }

  void tokenStream_data() {
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");
    QTest::newRow("newline") << "a|b" << "1:a 2: 1:b";
    QTest::newRow("italic tags") << "{y:i}x{/y:i}" << "5: 1:x 6:";
    QTest::newRow("leading whitespace dropped") << "  lead" << "1:lead";
    QTest::newRow("leading slash is italic") << "/ital" << "5: 1:ital";
    QTest::newRow("slash inside a word") << "w/slash" << "1:w/slash";
    QTest::newRow("tags are case-insensitive")
        << "{Y:B}{B}<B>{u}</U>" << "3: 3: 3: 7: 8:";
    QTest::newRow("unterminated colour") << "{c:abc" << "1:{c:abc";
    QTest::newRow("whitespace runs") << "x  y\t z" << "1:x 0: 1:y 0: 1:z";
    // only {c:...} has always been matched case-insensitively
    QTest::newRow("upper-case font tag")
        << "<FONT color=x>Y" << "1:<FONT 0: 1:color=x>Y";
    QTest::newRow("CRLF") << "a\r\nb" << "1:a 2: 1:b";
  }

  void tokenStream() {
    QFETCH(QString, input);
    QFETCH(QString, expected);
    QCOMPARE(tokens(registry->select("SRT")->decodeTokenStream(input)),
             expected);
  }

  void convert_data() {
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QString>("target");
    QTest::addColumn<QString>("expected");

    QTest::newRow("MicroDVD -> SRT")
        << microDvdSample << "srt"
        << QString::fromUtf8(
               "1\n00:00:01,000 --> 00:00:02,000\nWitaj\nświecie\n\n"
               "2\n00:00:03,000 --> 00:00:04,000\n<i>Drugi\n\n");
    QTest::newRow("MicroDVD -> MPL2")
        << microDvdSample << "mpl2"
        << QString::fromUtf8("[10][20]Witaj|świecie\n[30][40]{y:i}Drugi\n");
    QTest::newRow("MicroDVD -> TMPlayer")
        << microDvdSample << "tmp"
        << QString::fromUtf8("00:00:01: Witaj|świecie\n00:00:03: {y:i}Drugi\n");
    QTest::newRow("SRT -> MicroDVD")
        << subRipSample << "mDVD" << QString::fromUtf8("{25}{50}Zażółć\n");
    QTest::newRow("SRT -> SRT")
        << subRipSample << "srt"
        << QString::fromUtf8("1\n00:00:01,000 --> 00:00:02,000\nZażółć\n\n");
    QTest::newRow("MPL2 -> SRT")
        << mpl2Sample << "srt"
        << QString(
               "1\n00:00:01,000 --> 00:00:02,000\nPierwszy\n\n"
               "2\n00:00:03,000 --> 00:00:04,000\nB\n\n");
    QTest::newRow("MPL2 -> MicroDVD")
        << mpl2Sample << "mDVD" << QString("{25}{50}Pierwszy\n{75}{100}B\n");
    QTest::newRow("TMPlayer -> SRT")
        << tmPlayerSample << "srt"
        << QString(
               "1\n00:00:01,000 --> 00:00:06,000\nRaz\nDwa\n\n"
               "2\n00:00:09,000 --> 00:00:14,000\nTrzy\n\n");
    QTest::newRow("TMPlayer -> MicroDVD")
        << tmPlayerSample << "mDVD"
        << QString("{25}{150}Raz|Dwa\n{225}{350}Trzy\n");
    QTest::newRow("TMPlayer -> MPL2")
        << tmPlayerSample << "mpl2"
        << QString("[10][60]Raz|Dwa\n[90][140]Trzy\n");
  }

  void convert() {
    QFETCH(QByteArray, input);
    QFETCH(QString, target);
    QFETCH(QString, expected);
    bool ok = false;
    QCOMPARE(this->convert(input, target, true, 25.0, &ok), expected);
    QVERIFY(ok);
  }

  void convertAddsAdvertisement() {
    QString out = convert(microDvdSample, "srt", false);
    QVERIFY(
        out.endsWith("3\n00:00:06,000 --> 00:00:14,000\n"
                     "Subtitles downloaded and processed by QNapi\n" +
                     LibQNapi::webpageUrl() + "\n\n"));

    // converting again does not add a second one
    QString in = tmp.filePath("again.txt");
    QVERIFY(writeFile(in, out.toUtf8()));
    QVERIFY(converter(false)->convertSubtitles(in, "srt", in,
                                               tmp.filePath("movie.avi")));
    QCOMPARE(QString::fromUtf8(readFile(in)).replace("\r\n", "\n"), out);
  }

  void convertWithFpsRatioAndDelay() {
    QString in = tmp.filePath("fps.txt");
    QString out = tmp.filePath("fps-out.txt");
    QVERIFY(writeFile(in, microDvdSample));
    QVERIFY(
        converter(true)->convertSubtitles(in, "srt", out, 23.976, 1.5, -0.5));
    QCOMPARE(
        QString::fromUtf8(readFile(out)).replace("\r\n", "\n"),
        QString::fromUtf8("1\n00:00:01,063 --> 00:00:02,627\nWitaj\nświecie\n\n"
                          "2\n00:00:04,192 --> 00:00:05,755\n<i>Drugi\n\n"));

    QVERIFY(writeFile(in, tmPlayerSample));
    QVERIFY(
        converter(true)->convertSubtitles(in, "mDVD", out, 23.976, 1.5, -0.5));
    QCOMPARE(QString::fromUtf8(readFile(out)).replace("\r\n", "\n"),
             QString("{22}{201}Raz|Dwa\n{310}{489}Trzy\n"));
  }

  void convertNeedsFrameRateBetweenFrameAndTimeFormats() {
    bool ok = true;
    convert(microDvdSample, "srt", true, 0.0, &ok);
    QVERIFY(!ok);
    // same kind of format needs no frame rate
    convert(subRipSample, "tmp", true, 0.0, &ok);
    QVERIFY(ok);
  }

  void convertRejectsUnknownInput() {
    bool ok = true;
    convert("just some text\n", "srt", true, 25.0, &ok);
    QVERIFY(!ok);
  }

  void convertKeepsLegacyEncoding() {
    // windows-1250 "Zażółć"
    QString out;
    QString in = tmp.filePath("cp1250.txt");
    QVERIFY(writeFile(in, "[10][20]Za\xbf\xf3\xb3\xe6\n"));
    QVERIFY(converter(true)->convertSubtitles(in, "mDVD", in,
                                              tmp.filePath("movie.avi")));
    QCOMPARE(readFile(in).replace("\r\n", "\n"),
             QByteArray("{25}{50}Za\xbf\xf3\xb3\xe6\n"));
  }
};

QTEST_GUILESS_MAIN(TestSubConvert)
#include "tst_subconvert.moc"
