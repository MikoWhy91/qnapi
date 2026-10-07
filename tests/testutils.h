#ifndef QNAPI_TESTS_TESTUTILS_H
#define QNAPI_TESTS_TESTUTILS_H

#include <QByteArray>
#include <QFile>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

#include "config/qnapiconfig.h"
#include "engines/subtitledownloadengine.h"
#include "movieinfo/movieinfoprovider.h"

namespace TestUtils {

// pseudo-random bytes; tests/reference values were computed from the same
// generator with an independent implementation of each checksum
inline QByteArray pattern(qint64 size, quint32 seed = 7) {
  QByteArray data(int(size), '\0');
  quint32 x = seed;
  for (int i = 0; i < data.size(); ++i) {
    x = (x * 1103515245u + 12345u) & 0x7FFFFFFFu;
    data[i] = char((x >> 16) & 0xFF);
  }
  return data;
}

inline bool writeFile(const QString& path, const QByteArray& data) {
  QFile f(path);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
  return f.write(data) == data.size();
}

inline QByteArray readFile(const QString& path) {
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly)) return QByteArray();
  return f.readAll();
}

class FixedMovieInfoProvider : public MovieInfoProvider {
 public:
  explicit FixedMovieInfoProvider(double fps) : fps(fps) {}
  const Maybe<MovieInfo> getMovieInfo(const QString&) const {
    if (fps <= 0.0) return nothing();
    return just(MovieInfo(640, 480, fps, 100.0));
  }

 private:
  double fps;
};

// Engine whose search results are scripted per language.
class FakeEngine : public SubtitleDownloadEngine {
 public:
  struct Result {
    QString name;
    SubtitleResolution resolution;
  };

  FakeEngine(const QString& name, const QString& tmpPath)
      : SubtitleDownloadEngine(tmpPath),
        metadata(name, name, nothing(), nothing()) {}

  QMap<QString, QList<Result>> results;
  QString errorOnSearch;
  bool downloadSucceeds = true;
  QStringList searchedLanguages;
  QStringList downloadedNames;

  SubtitleDownloadEngineMetadata meta() const { return metadata; }
  const char* const* enginePixmapData() const { return nullptr; }

  QString checksum(QString) { return QString(); }

  bool lookForSubtitles(QString lang) {
    searchedLanguages << lang;
    for (const Result& r : results.value(lang)) {
      subtitlesList << SubtitleInfo(lang, metadata.name(), "fake:" + r.name,
                                    r.name, "", "srt", r.resolution);
    }
    return !results.value(lang).isEmpty();
  }

  QList<SubtitleInfo> listSubtitles() { return subtitlesList; }

  bool download(QUuid id) {
    Maybe<SubtitleInfo> s = resolveById(id);
    if (!s || !downloadSucceeds) return false;
    downloadedNames << s.value().name;
    return true;
  }

  bool unpack(QUuid id) {
    Maybe<SubtitleInfo> s = resolveById(id);
    if (!s) return false;
    subtitlesTmp = tmpPath + "/" + s.value().name + ".srt";
    return writeFile(subtitlesTmp, "1\n00:00:01,000 --> 00:00:02,000\n" +
                                       s.value().name.toUtf8() + "\n\n");
  }

  void cleanup() { subtitlesList.clear(); }

  QString lastError() const { return errorOnSearch; }

 private:
  SubtitleDownloadEngineMetadata metadata;
};

}  // namespace TestUtils

#endif  // QNAPI_TESTS_TESTUTILS_H
