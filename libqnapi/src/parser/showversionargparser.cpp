/*****************************************************************************
** QNapi
** Copyright (C) 2008-2017 Piotr Krzemiński <pio.krzeminski@gmail.com>
** Copyright (C) 2026 Mikołaj Stańczak <6730023+MikoWhy91@users.noreply.github.com>
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

#include "showversionargparser.h"
#include "qnapicommand.h"

ShowVersionArgParser::ShowVersionArgParser() {}

QVariant ShowVersionArgParser::parse(const QStringList& args,
                                     const QNapiConfig& config) const {
  if (args.contains("-v") || args.contains("--version")) {
    return QVariant::fromValue(ParsedCommand{
        config, QVariant::fromValue(QNapiCommand::ShowVersion())});
  } else {
    return QVariant::fromValue(NothingParsed());
  }
}

Maybe<CliArgParser::HelpInfo> ShowVersionArgParser::helpInfo() const {
  return just(HelpInfo{"-v", "--version", "", tr("Show version and exit")});
}
