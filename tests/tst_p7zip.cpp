#include <QDir>
#include <QProcess>
#include <QTemporaryDir>
#include <QtTest>

#include "testutils.h"
#include "utils/p7zipdecoder.h"

using TestUtils::readFile;
using TestUtils::writeFile;

// Runs P7ZipDecoder against a real 7-Zip binary (QNAPI_TEST_7ZIP), e.g. the
// one bundled with the Windows and macOS packages.
class TestP7Zip : public QObject {
  Q_OBJECT

  QTemporaryDir tmp;
  QString p7zip;
  QString archive;

 private slots:
  void initTestCase() {
    QVERIFY(tmp.isValid());
    p7zip = qEnvironmentVariable("QNAPI_TEST_7ZIP");
    if (p7zip.isEmpty()) QSKIP("QNAPI_TEST_7ZIP is not set");

    QDir().mkpath(tmp.filePath("src"));
    QVERIFY(writeFile(tmp.filePath("src/movie.txt"), "{1}{2}Napisy\n"));
    QVERIFY(writeFile(tmp.filePath("src/with space.srt"), "srt"));

    archive = tmp.filePath("napi.7z");
    QProcess process;
    process.setWorkingDirectory(tmp.filePath("src"));
    process.start(p7zip,
                  {"a", "-pSECRET", archive, "movie.txt", "with space.srt"});
    QVERIFY2(process.waitForFinished(30000), qPrintable(process.errorString()));
    QCOMPARE(process.exitCode(), 0);
  }

  void listArchiveFiles() {
    QStringList files = P7ZipDecoder(p7zip).listArchiveFiles(archive);
    // the first "Path =" entry is the archive itself
    QVERIFY(files.contains("movie.txt"));
    QVERIFY(files.contains("with space.srt"));
  }

  void unpackSecureArchive() {
    QString out = tmp.filePath("out");
    QDir().mkpath(out);
    QVERIFY(
        P7ZipDecoder(p7zip).unpackSecureArchiveFiles(archive, "SECRET", out));
    QCOMPARE(readFile(out + "/movie.txt"), QByteArray("{1}{2}Napisy\n"));
    QCOMPARE(readFile(out + "/with space.srt"), QByteArray("srt"));
  }

  void unpackWithWrongPasswordExtractsNothing() {
    QString out = tmp.filePath("wrong");
    QDir().mkpath(out);
    P7ZipDecoder(p7zip).unpackSecureArchiveFiles(archive, "WRONG", out);
    QVERIFY(readFile(out + "/movie.txt") != QByteArray("{1}{2}Napisy\n"));
  }
};

QTEST_GUILESS_MAIN(TestP7Zip)
#include "tst_p7zip.moc"
