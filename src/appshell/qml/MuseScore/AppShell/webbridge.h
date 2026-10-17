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
#include <QString>

#include <qqmlintegration.h>

//! The connection to the web page of the browser version (src/web). On every other platform
//! everything here does nothing: isWeb() is false and the functions return at once.
namespace mu::appshell::web {
//! True in the browser version (Qt for WebAssembly)
bool isWeb();

//! The second browser tab that shows the score to the audience (?mode=presentation). It plays nothing and
//! does not write any data.
bool isPresentationInstance();

//! Asks the page to write the files of the app (sessions, settings, score library) back to the storage
//! of the browser. Cheap to call: the page merges calls.
void persistStorage();

//! Scores that the page passes in (file dialog, shared or launched files) live in memory only. This copies
//! such a file to the score library (below the persistent data folder, one folder per content hash) and
//! returns its path, so a session finds its score again after a restart, whatever the name of the file is.
//! Other paths are returned as they are.
QString libraryPath(const QString& filePath);

//! The presentation is a browser tab of its own. Opening needs a click in the call chain, or the browser blocks it.
//! Returns false if the tab could not be opened.
bool openPresentationTab(const QString& scorePath);
void closePresentationTab();
}

namespace mu::appshell {
//! QML access to the page: the presentation tab, full screen and the backup of the sessions.
//!
//! Rehearsal tab:     openPresentationTab() / publishState() / closePresentationTab(), signal presentationClosed
//! Presentation tab:  signal stateReceived
//! The state is a JSON text that only the QML code of both tabs knows (see RehearsalPage.qml).
class WebBridge : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isWeb READ isWeb CONSTANT)
    Q_PROPERTY(bool isPresentationInstance READ isPresentationInstance CONSTANT)

    QML_ELEMENT

public:
    explicit WebBridge(QObject* parent = nullptr);
    ~WebBridge() override;

    bool isWeb() const;
    bool isPresentationInstance() const;

    //! Opens the presentation tab for the score at the path (must be called from a click, or the browser blocks it)
    Q_INVOKABLE bool openPresentationTab(const QString& scorePath);
    Q_INVOKABLE void closePresentationTab();
    Q_INVOKABLE void publishState(const QString& json);

    Q_INVOKABLE void toggleFullscreen();

    //! Backup of sessions.json (a file for the user), against cleared site data
    Q_INVOKABLE void exportSessions();
    Q_INVOKABLE void importSessions();

    //! Called from the page (via embind), for every bridge that exists
    static void deliverState(const QString& json);
    static void deliverPresentationClosed();

signals:
    void stateReceived(const QString& json);
    void presentationClosed();
};
}
