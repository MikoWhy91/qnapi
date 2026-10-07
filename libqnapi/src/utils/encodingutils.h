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

#ifndef ENCODINGUTILS_H
#define ENCODINGUTILS_H

#include <QByteArray>
#include <QString>
#include <QStringList>

class EncodingUtils {
 public:
  EncodingUtils();

  QString replaceDiacriticsWithASCII(const QString& str) const;
  QString detectBufferEncoding(const QByteArray& buffer) const;
  QString detectFileEncoding(const QString& filename) const;

  static bool isEncodingAvailable(const QString& encoding);
  static QStringList availableEncodings();

  // plain conversion with the named encoding; decode() drops a leading BOM
  static QString decode(const QByteArray& data, const QString& encoding);
  static QByteArray encode(const QString& text, const QString& encoding);

  // conversion as done by QTextStream: a Unicode BOM in the data overrides
  // the encoding, no BOM is written, unknown encodings fall back to the
  // locale encoding
  static QString decodeText(const QByteArray& data, const QString& encoding);
  static QByteArray encodeText(const QString& text, const QString& encoding);

 private:
  QString diacritics;
  QStringList replacements;
  QStringList codecs;
};

#endif  // ENCODINGUTILS_H
