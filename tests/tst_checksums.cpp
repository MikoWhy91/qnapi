#include <QTemporaryDir>
#include <QtTest>

#include "engines/napiprojektdownloadengine.h"
#include "engines/opensubtitlesdownloadengine.h"
#include "testutils.h"

using TestUtils::pattern;
using TestUtils::writeFile;

// Expected values come from an independent Python implementation of the
// OpenSubtitles hash and of NapiProjekt's MD5 + "f" digest, fed with the
// same TestUtils::pattern() data.
class TestChecksums : public QObject {
  Q_OBJECT

  QTemporaryDir tmp;

  QString openSubtitlesHash(const QString& path) {
    OpenSubtitlesDownloadEngine engine(tmp.path(), EngineConfig(),
                                       QSharedPointer<const P7ZipDecoder>(),
                                       "test", "en");
    return engine.checksum(path);
  }

  QString napiProjektHash(const QString& path) {
    NapiProjektDownloadEngine engine(tmp.path(), EngineConfig(),
                                     QSharedPointer<const P7ZipDecoder>());
    return engine.checksum(path);
  }

 private slots:
  void initTestCase() { QVERIFY(tmp.isValid()); }

  void openSubtitlesHash_data() {
    QTest::addColumn<int>("size");
    QTest::addColumn<QString>("expected");
    QTest::newRow("exactly 2 x 64 KiB") << 131072 << "53c63602aa5e325a";
    QTest::newRow("overlap-free 200000 B") << 200000 << "e17f970b89e17d12";
    QTest::newRow("1 MiB") << 1048576 << "77cd024c959c595a";
  }

  void openSubtitlesHash() {
    QFETCH(int, size);
    QFETCH(QString, expected);
    QString path = tmp.filePath(QString("movie%1.avi").arg(size));
    QVERIFY(writeFile(path, pattern(size)));
    QCOMPARE(openSubtitlesHash(path), expected);
  }

  // upstream #178: the size of files over 2 GiB was truncated
  void openSubtitlesHashOfFileOver4GiB() {
#ifdef Q_OS_WIN
    QSKIP("needs a sparse file, which QFile cannot create on NTFS");
#else
    const qint64 size = 5LL * 1024 * 1024 * 1024 + 4096;
    QString path = tmp.filePath("big.mkv");
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    if (!f.resize(size)) QSKIP("the file system cannot create sparse files");
    QVERIFY(f.seek(0));
    QCOMPARE(f.write(pattern(65536, 1)), qint64(65536));
    QVERIFY(f.seek(size - 65536));
    QCOMPARE(f.write(pattern(65536, 2)), qint64(65536));
    f.close();
    QCOMPARE(QFileInfo(path).size(), size);

    QCOMPARE(openSubtitlesHash(path), QString("3e65d68d67bb5c72"));
#endif
  }

  void openSubtitlesHashOfMissingFile() {
    QCOMPARE(openSubtitlesHash(tmp.filePath("missing.avi")), QString(""));
  }

  void napiProjektHash_data() {
    QTest::addColumn<int>("size");
    QTest::addColumn<QString>("expected");
    QTest::newRow("small file") << 1000 << "9605da2eed16f934cd58cea436ccf2ba";
    // only the first 10 MiB are hashed
    QTest::newRow("over 10 MiB")
        << (10 * 1024 * 1024 + 777) << "d0fd067432eed5ad0f8bc85cd8813a5a";
  }

  void napiProjektHash() {
    QFETCH(int, size);
    QFETCH(QString, expected);
    QString path = tmp.filePath(QString("napi%1.avi").arg(size));
    QVERIFY(writeFile(path, pattern(size)));
    QCOMPARE(napiProjektHash(path), expected);
  }

  void napiProjektFDigest_data() {
    QTest::addColumn<QString>("md5");
    QTest::addColumn<QString>("expected");
    QTest::newRow("empty file md5")
        << "d41d8cd98f00b204e9800998ecf8427e" << "8030b";
    QTest::newRow("hex sequence")
        << "0123456789abcdef0123456789abcdef" << "e2308";
    QTest::newRow("small file")
        << "9605da2eed16f934cd58cea436ccf2ba" << "a0806";
    QTest::newRow("over 10 MiB")
        << "d0fd067432eed5ad0f8bc85cd8813a5a" << "c2148";
    QTest::newRow("not an md5") << "abc" << "";
  }

  void napiProjektFDigest() {
    QFETCH(QString, md5);
    QFETCH(QString, expected);
    QCOMPARE(NapiProjektDownloadEngine::npFDigest(md5), expected);
  }
};

QTEST_GUILESS_MAIN(TestChecksums)
#include "tst_checksums.moc"
