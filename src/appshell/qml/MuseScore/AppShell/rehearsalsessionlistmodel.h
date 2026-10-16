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

#include <QAbstractListModel>
#include <qqmlintegration.h>

namespace mu::appshell {
//! The sessions of the user for the start page, most recently opened first
class RehearsalSessionListModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

    QML_ELEMENT

public:
    explicit RehearsalSessionListModel(QObject* parent = nullptr);

    enum Roles {
        SessionIdRole = Qt::UserRole + 1, // "sessionId"
        TitleRole,                        // "title"
        FilePathRole,                     // "filePath"
        FileExistsRole,                   // "fileExists": false if the file was moved or deleted
        LastOpenedRole                    // "lastOpened": localized date and time
    };

    QVariant data(const QModelIndex& index, int role) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    //! Re-checks which files still exist (they can change while the app is running)
    Q_INVOKABLE void refresh();

signals:
    void countChanged();
};
}
