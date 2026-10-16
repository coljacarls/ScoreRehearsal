/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * ScoreRehearsal
 * Copyright (C) 2026 Colja Carls
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <QObject>
#include <QRect>
#include <QString>
#include <qqmlintegration.h>

#include "modularity/ioc.h"
#include "ui/imainwindow.h"

namespace mu::appshell {
//! The state of the main window that is restored at the next start: windowed (with its size and position),
//! maximized or full screen. For maximized and full screen it also keeps the size and position of the windowed state.
class WindowStateModel : public QObject, public muse::Contextable
{
    Q_OBJECT

    //! "windowed", "maximized" or "fullscreen"
    Q_PROPERTY(QString state READ state CONSTANT)
    //! The windowed geometry (empty if there is none saved yet)
    Q_PROPERTY(QRect geometry READ geometry CONSTANT)
    //! Whether the full screen state was entered from the maximized state (and so leads back to it)
    Q_PROPERTY(bool maximizedBeforeFullScreen READ maximizedBeforeFullScreen CONSTANT)

    QML_ELEMENT

public:
    explicit WindowStateModel(QObject* parent = nullptr);

    QString state() const;
    QRect geometry() const;
    bool maximizedBeforeFullScreen() const;

    Q_INVOKABLE void save(const QString& state, const QRect& geometry, bool maximizedBeforeFullScreen);
    Q_INVOKABLE void enterFullScreen();

private:
    muse::ContextInject<muse::ui::IMainWindow> mainWindow = { this };
};
}
