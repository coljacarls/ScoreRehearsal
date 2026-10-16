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

#include "rehearsalsessionlistmodel.h"

#include <QDir>
#include <QFileInfo>
#include <QLocale>

#include "rehearsalsessionstore.h"

using namespace mu::appshell;

RehearsalSessionListModel::RehearsalSessionListModel(QObject* parent)
    : QAbstractListModel(parent)
{
    connect(&RehearsalSessionStore::instance(), &RehearsalSessionStore::listChanged, this, [this]() {
        refresh();
    });
}

QVariant RehearsalSessionListModel::data(const QModelIndex& index, int role) const
{
    const QList<RehearsalSession>& sessions = RehearsalSessionStore::instance().sessions();
    if (!index.isValid() || index.row() >= sessions.size()) {
        return QVariant();
    }

    const RehearsalSession& session = sessions.at(index.row());

    switch (role) {
    case SessionIdRole: return session.id;
    case TitleRole: return session.title;
    case FilePathRole: return QDir::toNativeSeparators(session.filePath);
    case FileExistsRole: return QFileInfo::exists(session.filePath);
    case LastOpenedRole: return QLocale().toString(session.lastOpened, QLocale::ShortFormat);
    }

    return QVariant();
}

int RehearsalSessionListModel::rowCount(const QModelIndex&) const
{
    return static_cast<int>(RehearsalSessionStore::instance().sessions().size());
}

QHash<int, QByteArray> RehearsalSessionListModel::roleNames() const
{
    static const QHash<int, QByteArray> roles {
        { SessionIdRole, "sessionId" },
        { TitleRole, "title" },
        { FilePathRole, "filePath" },
        { FileExistsRole, "fileExists" },
        { LastOpenedRole, "lastOpened" }
    };

    return roles;
}

void RehearsalSessionListModel::refresh()
{
    beginResetModel();
    endResetModel();

    emit countChanged();
}
