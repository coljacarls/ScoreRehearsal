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

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>

namespace mu::appshell {
//! What the app remembers about one score file: a reference to the file (a path, which can be
//! re-linked) and everything that was set up while rehearsing with it.
//! The score itself is never touched; a changed file is simply loaded again the next time.
struct RehearsalSession {
    QString id;
    QString filePath;
    QString title;
    QDateTime lastOpened;

    bool continuousView = true;
    bool tracksPanelVisible = true;
    //! Name of the selected part (empty: the full score)
    QString partName;

    //! Position of the playhead; only the numbers are kept (they may not match a changed score any more)
    int playheadMeasureIndex = 0;

    //! Opaque to the store, written and read by the rehearsal mixer
    QJsonObject mixer;

    //! Annotations of the laser pointer (strokes, highlight) per part and view mode (their coordinates
    //! belong to a layout): key -> { "strokes": [[x0, y0, x1, y1, ...], ...], "highlight": [x, y, w, h] }
    QJsonObject annotations;
};

//! The sessions of the user, persisted as JSON in the user-level app data folder.
//! A session is created the first time a score file is opened.
class RehearsalSessionStore : public QObject
{
    Q_OBJECT

public:
    static RehearsalSessionStore& instance();

    //! Sessions, most recently opened first
    const QList<RehearsalSession>& sessions() const;

    RehearsalSession* findById(const QString& id);
    RehearsalSession* findByPath(const QString& filePath);

    //! Returns the session of the file, creates it if there is none yet
    RehearsalSession& ensureForPath(const QString& filePath, const QString& title);

    //! Marks the session as the most recently opened one
    void touch(const QString& id);
    void setTitle(const QString& id, const QString& title);
    //! Points the session to another file (the old one was moved or deleted)
    void relink(const QString& id, const QString& newFilePath);
    void remove(const QString& id);

    //! Call after a session was modified in place; writes the file soon
    void markChanged(const QString& id);

    //! Writes pending changes immediately
    void flush();

    //! Where the sessions are stored (changeable for tests)
    void setStoragePath(const QString& path);
    QString storagePath() const;

    static QString normalizedPath(const QString& filePath);

signals:
    //! The set of sessions, a title, a path or the order changed (not the session content)
    void listChanged();

private:
    RehearsalSessionStore();

    void ensureLoaded();
    void load();
    void save();

    QString m_storagePath;
    QList<RehearsalSession> m_sessions;
    QTimer m_saveTimer;
    bool m_loaded = false;
    bool m_dirty = false;
};
}
