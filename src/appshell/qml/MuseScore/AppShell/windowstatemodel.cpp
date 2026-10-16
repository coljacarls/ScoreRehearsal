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

#include "windowstatemodel.h"

#include "settings.h"

using namespace mu::appshell;

namespace {
const muse::Settings::Key STATE_KEY("appshell", "window/state");
const muse::Settings::Key MAXIMIZED_BEFORE_FULLSCREEN_KEY("appshell", "window/maximizedBeforeFullScreen");
const muse::Settings::Key X_KEY("appshell", "window/x");
const muse::Settings::Key Y_KEY("appshell", "window/y");
const muse::Settings::Key WIDTH_KEY("appshell", "window/width");
const muse::Settings::Key HEIGHT_KEY("appshell", "window/height");

const QString WINDOWED = QStringLiteral("windowed");
const QString MAXIMIZED = QStringLiteral("maximized");
const QString FULLSCREEN = QStringLiteral("fullscreen");
}

WindowStateModel::WindowStateModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

QString WindowStateModel::state() const
{
    const QString state = QString::fromStdString(muse::settings()->value(STATE_KEY).toString());
    return (state == MAXIMIZED || state == FULLSCREEN) ? state : WINDOWED;
}

QRect WindowStateModel::geometry() const
{
    muse::Settings* s = muse::settings();
    return QRect(s->value(X_KEY).toInt(), s->value(Y_KEY).toInt(), s->value(WIDTH_KEY).toInt(), s->value(HEIGHT_KEY).toInt());
}

bool WindowStateModel::maximizedBeforeFullScreen() const
{
    return muse::settings()->value(MAXIMIZED_BEFORE_FULLSCREEN_KEY).toBool();
}

void WindowStateModel::save(const QString& state, const QRect& geometry, bool maximizedBeforeFullScreen)
{
    muse::Settings* s = muse::settings();
    s->setSharedValue(STATE_KEY, muse::Val(state.toStdString()));
    s->setSharedValue(MAXIMIZED_BEFORE_FULLSCREEN_KEY, muse::Val(maximizedBeforeFullScreen));

    if (geometry.isValid()) {
        s->setSharedValue(X_KEY, muse::Val(geometry.x()));
        s->setSharedValue(Y_KEY, muse::Val(geometry.y()));
        s->setSharedValue(WIDTH_KEY, muse::Val(geometry.width()));
        s->setSharedValue(HEIGHT_KEY, muse::Val(geometry.height()));
    }
}

//! Goes through the main window, so that leaving the full screen leads back to the state before it
void WindowStateModel::enterFullScreen()
{
    if (!mainWindow()->isFullScreen()) {
        mainWindow()->toggleFullScreen();
    }
}
