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

#include "aboutmodel.h"

#include <QClipboard>
#include <QGuiApplication>

using namespace mu::appshell;

AboutModel::AboutModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

QString AboutModel::appVersion() const
{
    return QString::fromStdString(configuration()->appVersion());
}

QString AboutModel::museScoreVersion() const
{
    return QString::fromStdString(configuration()->museScoreVersion());
}

QString AboutModel::museScoreRevision() const
{
    return QString::fromStdString(configuration()->museScoreRevision());
}

QString AboutModel::sourceCodeUrl() const
{
    return QString::fromStdString(configuration()->sourceCodeUrl());
}

void AboutModel::copyRevisionToClipboard() const
{
    QGuiApplication::clipboard()->setText(museScoreRevision());
}
