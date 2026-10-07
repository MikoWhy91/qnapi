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

#include <QDesktopServices>
#include <QDesktopWidget>

#include "engines/opensubtitlesdownloadengine.h"
#include "frmopensubtitlesconfig.h"

frmOpenSubtitlesConfig::frmOpenSubtitlesConfig(const EngineConfig &config,
                                               QWidget *parent,
                                               Qt::WindowFlags f)
    : QDialog(parent, f), config(config) {
  ui.setupUi(this);

  ui.leApiKey->setText(config.apiKey());
  ui.leNick->setText(config.nick());
  ui.lePass->setText(config.password());

  QIcon openSubtitlesIcon =
      QIcon(QPixmap(OpenSubtitlesDownloadEngine::pixmapData));
  setWindowIcon(openSubtitlesIcon);

  connect(ui.pbRegister, SIGNAL(clicked()), this, SLOT(pbRegisterClicked()));
  connect(ui.pbGetApiKey, SIGNAL(clicked()), this, SLOT(pbGetApiKeyClicked()));

  QRect position = frameGeometry();
  position.moveCenter(QDesktopWidget().availableGeometry().center());
  move(position.topLeft());
}

EngineConfig frmOpenSubtitlesConfig::getConfig() const { return config; }

void frmOpenSubtitlesConfig::accept() {
  config = config.setNick(ui.leNick->text())
               .setPassword(ui.lePass->text())
               .setApiKey(ui.leApiKey->text().trimmed());
  QDialog::accept();
}

void frmOpenSubtitlesConfig::pbGetApiKeyClicked() {
  QDesktopServices::openUrl(OpenSubtitlesDownloadEngine::apiKeyUrl);
}

void frmOpenSubtitlesConfig::pbRegisterClicked() {
  Maybe<QUrl> maybeRegistrationUrl =
      OpenSubtitlesDownloadEngine::metadata.registrationUrl();

  if (maybeRegistrationUrl) {
    QDesktopServices::openUrl(maybeRegistrationUrl.value());
  }
}
