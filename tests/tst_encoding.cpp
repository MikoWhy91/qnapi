#include <QTemporaryDir>
#include <QtTest>

#include "libqnapi.h"
#include "subtitlepostprocessor.h"
#include "testutils.h"
#include "utils/encodingutils.h"

using TestUtils::readFile;
using TestUtils::writeFile;

namespace {
const QString polish = QString::fromUtf8("Zażółć gęślą jaźń");
const QByteArray polishUtf8 = polish.toUtf8();
const QByteArray polishCp1250 = "Za\xbf\xf3\xb3\xe6 g\xea\x9cl\xb9 ja\x9f\xf1";
const QByteArray polishIso88592 =
    "Za\xbf\xf3\xb3\xe6 g\xea\xb6l\xb1 ja\xbc\xf1";
const QByteArray utf8Bom = "\xef\xbb\xbf";
}  // namespace

class TestEncoding : public QObject {
  Q_OBJECT

  QTemporaryDir tmp;

  PostProcessingConfig ppConfig(EncodingChangeMethod method,
                                const QString& from, bool autoDetect,
                                const QString& to) {
    return PostProcessingConfig(true, method, from, autoDetect, to, false, "",
                                "", true, false, QStringList());
  }

  QByteArray postProcess(const QByteArray& content,
                         const PostProcessingConfig& config) {
    QString path = tmp.filePath("subtitles.txt");
    writeFile(path, content);
    SubtitlePostProcessor(config, LibQNapi::subtitleConverter(config))
        .perform(tmp.filePath("movie.avi"), path);
    return readFile(path);
  }

 private slots:
  void initTestCase() { QVERIFY(tmp.isValid()); }

  void detectBufferEncoding_data() {
    QTest::addColumn<QByteArray>("buffer");
    QTest::addColumn<QString>("encoding");
    QTest::newRow("ASCII") << QByteArray("plain ascii") << "UTF-8";
    QTest::newRow("UTF-8") << polishUtf8 << "UTF-8";
    QTest::newRow("UTF-8 with BOM") << utf8Bom + polishUtf8 << "UTF-8";
    QTest::newRow("windows-1250") << polishCp1250 << "windows-1250";
    QTest::newRow("ISO-8859-2") << polishIso88592 << "ISO-8859-2";
  }

  void detectBufferEncoding() {
    QFETCH(QByteArray, buffer);
    QFETCH(QString, encoding);
    QCOMPARE(EncodingUtils().detectBufferEncoding(buffer), encoding);
  }

  void detectFileEncoding() {
    QString path = tmp.filePath("detect.txt");
    QVERIFY(writeFile(path, polishCp1250));
    QCOMPARE(EncodingUtils().detectFileEncoding(path), QString("windows-1250"));
    QCOMPARE(EncodingUtils().detectFileEncoding(tmp.filePath("missing")),
             QString(""));
  }

  void replaceDiacritics() {
    QCOMPARE(EncodingUtils().replaceDiacriticsWithASCII(
                 QString::fromUtf8("Zażółć GĘŚLĄ jaźń ÀÿßŒ")),
             QString("Zazolc GESLA jazn AysOE"));
  }

  void availableEncodings() {
    const QStringList encodings = EncodingUtils::availableEncodings();
    for (const char* e : {"UTF-8", "windows-1250", "ISO-8859-2", "windows-1257",
                          "ISO-8859-13", "ISO-8859-16"}) {
      QVERIFY2(encodings.contains(e), e);
      QVERIFY(EncodingUtils::isEncodingAvailable(e));
    }
    QVERIFY(!EncodingUtils::isEncodingAvailable("no-such-encoding"));
    QVERIFY(!EncodingUtils::isEncodingAvailable(""));
  }

  void decodeAndEncode_data() {
    QTest::addColumn<QString>("encoding");
    QTest::addColumn<QByteArray>("bytes");
    QTest::newRow("UTF-8") << "UTF-8" << polishUtf8;
    QTest::newRow("windows-1250") << "windows-1250" << polishCp1250;
    QTest::newRow("ISO-8859-2") << "ISO-8859-2" << polishIso88592;
  }

  void decodeAndEncode() {
    QFETCH(QString, encoding);
    QFETCH(QByteArray, bytes);
    QCOMPARE(EncodingUtils::decode(bytes, encoding), polish);
    QCOMPARE(EncodingUtils::encode(polish, encoding), bytes);
    QCOMPARE(EncodingUtils::decodeText(bytes, encoding), polish);
    QCOMPARE(EncodingUtils::encodeText(polish, encoding), bytes);
  }

  void decodeTextFollowsUnicodeBom() {
    QByteArray utf16 = QByteArray("\xff\xfe") +
                       QByteArray(reinterpret_cast<const char*>(polish.utf16()),
                                  polish.size() * 2);
    QCOMPARE(EncodingUtils::decodeText(utf16, "windows-1250"), polish);
    QCOMPARE(EncodingUtils::decodeText(utf8Bom + polishUtf8, "UTF-8"), polish);
  }

  void encodeTextWritesNoBom() {
    QByteArray utf16 = EncodingUtils::encodeText("ab", "UTF-16LE");
    QCOMPARE(utf16, QByteArray("a\0b\0", 4));
  }

  void changeEncoding_data() {
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QString>("from");
    QTest::addColumn<QString>("to");
    QTest::addColumn<QByteArray>("expected");
    QTest::newRow("windows-1250 -> UTF-8")
        << polishCp1250 << "windows-1250" << "UTF-8" << polishUtf8;
    QTest::newRow("ISO-8859-2 -> UTF-8")
        << polishIso88592 << "ISO-8859-2" << "UTF-8" << polishUtf8;
    QTest::newRow("UTF-8 -> windows-1250")
        << polishUtf8 << "UTF-8" << "windows-1250" << polishCp1250;
    QTest::newRow("UTF-8 -> ISO-8859-2")
        << polishUtf8 << "UTF-8" << "ISO-8859-2" << polishIso88592;
    QTest::newRow("UTF-8 -> UTF-8")
        << polishUtf8 << "UTF-8" << "UTF-8" << polishUtf8;
    QTest::newRow("unknown source encoding leaves the file alone")
        << polishCp1250 << "no-such-encoding" << "UTF-8" << polishCp1250;
  }

  void changeEncoding() {
    QFETCH(QByteArray, input);
    QFETCH(QString, from);
    QFETCH(QString, to);
    QFETCH(QByteArray, expected);
    QCOMPARE(postProcess(input, ppConfig(ECM_CHANGE, from, false, to)),
             expected);
  }

  void decodeDropsBom() {
    QCOMPARE(EncodingUtils::decode(utf8Bom + polishUtf8, "UTF-8"), polish);
    QByteArray utf16 = QByteArray("\xff\xfe") +
                       QByteArray(reinterpret_cast<const char*>(polish.utf16()),
                                  polish.size() * 2);
    QCOMPARE(EncodingUtils::decode(utf16, "UTF-16"), polish);
  }

  void autoDetectedEncodingIsUsed() {
    // the configured source encoding is only a fallback
    QCOMPARE(postProcess(polishCp1250,
                         ppConfig(ECM_CHANGE, "ISO-8859-2", true, "UTF-8")),
             polishUtf8);
    QCOMPARE(postProcess(polishIso88592,
                         ppConfig(ECM_CHANGE, "windows-1250", true, "UTF-8")),
             polishUtf8);
    QCOMPARE(postProcess(polishUtf8, ppConfig(ECM_CHANGE, "windows-1250", true,
                                              "windows-1250")),
             polishCp1250);
  }

  // upstream #200: BOM left behind when converting UTF-8+BOM to UTF-8
  void utf8BomIsRemoved_data() {
    QTest::addColumn<QString>("from");
    QTest::addColumn<bool>("autoDetect");
    QTest::addColumn<QString>("to");
    QTest::addColumn<QByteArray>("expected");
    QTest::newRow("default settings")
        << "windows-1250" << true << "UTF-8" << polishUtf8;
    QTest::newRow("explicit UTF-8")
        << "UTF-8" << false << "UTF-8" << polishUtf8;
    QTest::newRow("to windows-1250")
        << "UTF-8" << false << "windows-1250" << polishCp1250;
    QTest::newRow("detected, to ISO-8859-2")
        << "windows-1250" << true << "ISO-8859-2" << polishIso88592;
  }

  void utf8BomIsRemoved() {
    QFETCH(QString, from);
    QFETCH(bool, autoDetect);
    QFETCH(QString, to);
    QFETCH(QByteArray, expected);
    QCOMPARE(postProcess(utf8Bom + polishUtf8,
                         ppConfig(ECM_CHANGE, from, autoDetect, to)),
             expected);
  }

  void utf8BomIsRemovedWithFormatConversion() {
    PostProcessingConfig config(true, ECM_CHANGE, "windows-1250", true, "UTF-8",
                                false, "srt", "", true, false, QStringList());
    QByteArray output =
        postProcess(utf8Bom + "1\r\n00:00:01,000 --> 00:00:02,000\r\n" +
                        polishUtf8 + "\r\n\r\n",
                    config);
    QCOMPARE(output.replace("\r\n", "\n"),
             "1\n00:00:01,000 --> 00:00:02,000\n" + polishUtf8 + "\n\n");
  }

  void replaceDiacriticsInFile() {
    QCOMPARE(postProcess(polishCp1250,
                         ppConfig(ECM_REPLACE_DIACRITICS, "", false, "")),
             QByteArray("Zazolc gesla jazn"));
  }

  void originalEncodingIsKept() {
    QByteArray input = utf8Bom + polishUtf8;
    QCOMPARE(postProcess(input, ppConfig(ECM_ORIGINAL, "", false, "")), input);
  }

  void removeLinesContainingWords() {
    PostProcessingConfig config(true, ECM_ORIGINAL, "windows-1250", false, "",
                                false, "", "", true, true,
                                QStringList({"movie info", "", "SYNCHRO"}));
    QByteArray input =
        "{1}{2}Movie Info: xyz\r\n{3}{4}Za\xbf\xf3\xb3\xe6\r\n"
        "{5}{6}synchro: ktos\r\n\r\n{7}{8}Koniec\r\n";
    QByteArray output = postProcess(input, config).replace("\r\n", "\n");
    QCOMPARE(output,
             QByteArray("{3}{4}Za\xbf\xf3\xb3\xe6\n\n{7}{8}Koniec\n\n"));
  }
};

QTEST_GUILESS_MAIN(TestEncoding)
#include "tst_encoding.moc"
