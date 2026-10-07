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

#ifndef ENGINECONFIG_H
#define ENGINECONFIG_H

#include <QString>

class EngineConfig {
 private:
  QString nick_;
  QString password_;
  QString apiKey_;

 public:
  EngineConfig(const QString& nick = "", const QString& password = "",
               const QString& apiKey = "")
      : nick_(nick), password_(password), apiKey_(apiKey) {}

  static EngineConfig empty;

  QString nick() const { return nick_; }
  QString password() const { return password_; }
  QString apiKey() const { return apiKey_; }

  const EngineConfig setNick(const QString& nick) const {
    return EngineConfig(nick, password_, apiKey_);
  }
  const EngineConfig setPassword(const QString& password) const {
    return EngineConfig(nick_, password, apiKey_);
  }
  const EngineConfig setApiKey(const QString& apiKey) const {
    return EngineConfig(nick_, password_, apiKey);
  }

  QString toString() const;
};

#endif
