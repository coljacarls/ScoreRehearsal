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

#include "rehearsalsessionstore.h"

#include <algorithm>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>

#include "log.h"

using namespace mu::appshell;

static constexpr int STORE_VERSION = 1;
static constexpr int SAVE_DELAY_MS = 1000;

RehearsalSessionStore& RehearsalSessionStore::instance()
{
    static RehearsalSessionStore s_instance;
    return s_instance;
}

RehearsalSessionStore::RehearsalSessionStore()
{
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(SAVE_DELAY_MS);
    connect(&m_saveTimer, &QTimer::timeout, this, [this]() { save(); });

    if (QCoreApplication* app = QCoreApplication::instance()) {
        connect(app, &QCoreApplication::aboutToQuit, this, [this]() { flush(); });
    }
}

//! "Song.mscz" -> "Song"
static QString plainTitle(const QString& title)
{
    const QString suffix = QFileInfo(title).suffix().toLower();
    return suffix == QLatin1String("mscz") || suffix == QLatin1String("mscx") ? QFileInfo(title).completeBaseName() : title;
}

QString RehearsalSessionStore::normalizedPath(const QString& filePath)
{
    if (filePath.isEmpty()) {
        return QString();
    }

    return QDir::cleanPath(QFileInfo(filePath).absoluteFilePath());
}

void RehearsalSessionStore::setStoragePath(const QString& path)
{
    m_storagePath = path;
    m_sessions.clear();
    m_loaded = false;
    m_dirty = false;
}

QString RehearsalSessionStore::storagePath() const
{
    if (!m_storagePath.isEmpty()) {
        return m_storagePath;
    }

    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(dir).filePath(QStringLiteral("sessions.json"));
}

void RehearsalSessionStore::ensureLoaded()
{
    if (!m_loaded) {
        load();
    }
}

const QList<RehearsalSession>& RehearsalSessionStore::sessions() const
{
    const_cast<RehearsalSessionStore*>(this)->ensureLoaded();
    return m_sessions;
}

RehearsalSession* RehearsalSessionStore::findById(const QString& id)
{
    ensureLoaded();

    for (RehearsalSession& session : m_sessions) {
        if (session.id == id) {
            return &session;
        }
    }

    return nullptr;
}

RehearsalSession* RehearsalSessionStore::findByPath(const QString& filePath)
{
    ensureLoaded();

    const QString path = normalizedPath(filePath);
    if (path.isEmpty()) {
        return nullptr;
    }

#ifdef Q_OS_WIN
    static constexpr Qt::CaseSensitivity sensitivity = Qt::CaseInsensitive;
#else
    static constexpr Qt::CaseSensitivity sensitivity = Qt::CaseSensitive;
#endif

    for (RehearsalSession& session : m_sessions) {
        if (normalizedPath(session.filePath).compare(path, sensitivity) == 0) {
            return &session;
        }
    }

    return nullptr;
}

RehearsalSession& RehearsalSessionStore::ensureForPath(const QString& filePath, const QString& title)
{
    if (RehearsalSession* existing = findByPath(filePath)) {
        return *existing;
    }

    RehearsalSession session;
    session.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    session.filePath = normalizedPath(filePath);
    session.title = title.isEmpty() ? QFileInfo(filePath).completeBaseName() : plainTitle(title);
    session.lastOpened = QDateTime::currentDateTime();

    m_sessions.prepend(session);
    m_dirty = true;
    m_saveTimer.start();
    emit listChanged();

    return m_sessions.first();
}

void RehearsalSessionStore::touch(const QString& id)
{
    ensureLoaded();

    const auto it = std::find_if(m_sessions.begin(), m_sessions.end(), [&id](const RehearsalSession& s) {
        return s.id == id;
    });

    if (it == m_sessions.end()) {
        return;
    }

    it->lastOpened = QDateTime::currentDateTime();
    if (it != m_sessions.begin()) {
        std::rotate(m_sessions.begin(), it, it + 1);
    }

    markChanged(id);
    emit listChanged();
}

void RehearsalSessionStore::setTitle(const QString& id, const QString& title)
{
    RehearsalSession* session = findById(id);
    const QString newTitle = plainTitle(title);
    if (!session || newTitle.isEmpty() || session->title == newTitle) {
        return;
    }

    session->title = newTitle;
    markChanged(id);
    emit listChanged();
}

void RehearsalSessionStore::relink(const QString& id, const QString& newFilePath)
{
    RehearsalSession* session = findById(id);
    if (!session) {
        return;
    }

    session->filePath = normalizedPath(newFilePath);
    markChanged(id);
    emit listChanged();
}

void RehearsalSessionStore::remove(const QString& id)
{
    ensureLoaded();

    const qsizetype removed = m_sessions.removeIf([&id](const RehearsalSession& s) {
        return s.id == id;
    });

    if (removed == 0) {
        return;
    }

    m_dirty = true;
    m_saveTimer.start();
    emit listChanged();
}

void RehearsalSessionStore::markChanged(const QString&)
{
    m_dirty = true;
    m_saveTimer.start();
}

void RehearsalSessionStore::flush()
{
    m_saveTimer.stop();
    if (m_dirty) {
        save();
    }
}

void RehearsalSessionStore::load()
{
    m_loaded = true;
    m_sessions.clear();

    QFile file(storagePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        LOGE() << "unable to read the sessions: " << error.errorString();
        return;
    }

    const QJsonArray array = doc.object().value("sessions").toArray();
    for (const QJsonValue& value : array) {
        const QJsonObject o = value.toObject();

        RehearsalSession session;
        session.id = o.value("id").toString();
        session.filePath = o.value("filePath").toString();
        if (session.id.isEmpty() || session.filePath.isEmpty()) {
            continue;
        }

        session.title = o.value("title").toString(QFileInfo(session.filePath).completeBaseName());
        session.lastOpened = QDateTime::fromString(o.value("lastOpened").toString(), Qt::ISODate);
        session.continuousView = o.value("continuousView").toBool(true);
        session.tracksPanelVisible = o.value("tracksPanelVisible").toBool(true);
        session.playheadMeasureIndex = std::max(0, o.value("playheadMeasureIndex").toInt());
        session.mixer = o.value("mixer").toObject();

        m_sessions.append(session);
    }

    std::stable_sort(m_sessions.begin(), m_sessions.end(), [](const RehearsalSession& a, const RehearsalSession& b) {
        return a.lastOpened > b.lastOpened;
    });
}

void RehearsalSessionStore::save()
{
    m_dirty = false;

    QJsonArray array;
    for (const RehearsalSession& session : m_sessions) {
        QJsonObject o;
        o["id"] = session.id;
        o["filePath"] = session.filePath;
        o["title"] = session.title;
        o["lastOpened"] = session.lastOpened.toString(Qt::ISODate);
        o["continuousView"] = session.continuousView;
        o["tracksPanelVisible"] = session.tracksPanelVisible;
        o["playheadMeasureIndex"] = session.playheadMeasureIndex;
        o["mixer"] = session.mixer;
        array.append(o);
    }

    QJsonObject root;
    root["version"] = STORE_VERSION;
    root["sessions"] = array;

    const QString path = storagePath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        LOGE() << "unable to write the sessions to " << path;
        return;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        LOGE() << "unable to write the sessions to " << path;
    }
}
