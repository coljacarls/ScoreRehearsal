/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited and others
 * Modifications for ScoreRehearsal: Copyright (C) 2026 Colja Carls
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
#include <QStringList>
#include <QUrl>
#include <qqmlintegration.h>

#include "modularity/ioc.h"
#include "async/asyncable.h"

#include "actions/iactionsdispatcher.h"
#include "context/iglobalcontext.h"
#include "interactive/iinteractive.h"
#include "rcommand/icommanddispatcher.h"
#include "ui/imainwindow.h"
#include "ui/iuiconfiguration.h"
#include "notation/inotationconfiguration.h"
#include "notation/types/viewmode.h"
#include "playback/iplaybackcontroller.h"
#include "project/iopenprojectscenario.h"

namespace mu::appshell {
//! Page-level logic of the rehearsal page (adapted from MuseScore's RehearsalPage).
//! The view mode is session-only: it is applied by the rehearsal NotationView (viewModeOverride)
//! and never stored in the project. The session state (view mode, tracks panel visibility)
//! is shared by all instances (the page, the floating immersive bar).
class RehearsalPageModel : public QObject, public muse::Contextable, public muse::async::Asyncable
{
    Q_OBJECT

    Q_PROPERTY(bool hasProject READ hasProject NOTIFY hasProjectChanged)
    Q_PROPERTY(bool isContinuousView READ isContinuousView WRITE setIsContinuousView NOTIFY isContinuousViewChanged)
    Q_PROPERTY(bool isPresentationWindowOpen READ isPresentationWindowOpen WRITE setIsPresentationWindowOpen
               NOTIFY isPresentationWindowOpenChanged)
    Q_PROPERTY(int viewModeOverride READ viewModeOverride NOTIFY isContinuousViewChanged)
    Q_PROPERTY(bool isTracksPanelVisible READ isTracksPanelVisible WRITE setIsTracksPanelVisible NOTIFY isTracksPanelVisibleChanged)
    Q_PROPERTY(bool isFullScreen READ isFullScreen NOTIFY isFullScreenChanged)
    Q_PROPERTY(bool isDarkTheme READ isDarkTheme NOTIFY isDarkThemeChanged)
    Q_PROPERTY(QStringList partNames READ partNames NOTIFY partsChanged)
    Q_PROPERTY(int currentPartIndex READ currentPartIndex NOTIFY currentPartChanged)
    Q_PROPERTY(QString scoreTitle READ scoreTitle NOTIFY scoreTitleChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
    Q_PROPERTY(bool isPlayAllowed READ isPlayAllowed NOTIFY isPlayAllowedChanged)
    Q_PROPERTY(bool isMetronomeEnabled READ isMetronomeEnabled NOTIFY isMetronomeEnabledChanged)
    Q_PROPERTY(int measureNumber READ measureNumber NOTIFY playPositionChanged)
    Q_PROPERTY(int beatNumber READ beatNumber NOTIFY playPositionChanged)

    QML_ELEMENT

    muse::GlobalInject<notation::INotationConfiguration> notationConfiguration;
    muse::GlobalInject<muse::ui::IUiConfiguration> uiConfiguration;
    muse::ContextInject<context::IGlobalContext> globalContext = { this };
    muse::ContextInject<playback::IPlaybackController> playbackController = { this };
    muse::ContextInject<project::IOpenProjectScenario> openProjectScenario = { this };
    muse::ContextInject<muse::IInteractive> interactive = { this };
    muse::ContextInject<muse::rcommand::ICommandDispatcher> dispatcher = { this };
    muse::ContextInject<muse::actions::IActionsDispatcher> actionsDispatcher = { this };
    muse::ContextInject<muse::ui::IMainWindow> mainWindow = { this };

public:
    explicit RehearsalPageModel(QObject* parent = nullptr);

    Q_INVOKABLE void load();

    bool hasProject() const;

    bool isContinuousView() const;
    void setIsContinuousView(bool continuous);
    int viewModeOverride() const;

    //! The presentation window needs the continuous view (the view mode belongs to the score,
    //! not to a single view): while it is open, the rehearsal view stays in the continuous view
    bool isPresentationWindowOpen() const;
    void setIsPresentationWindowOpen(bool open);

    bool isTracksPanelVisible() const;
    void setIsTracksPanelVisible(bool visible);

    bool isFullScreen() const;
    Q_INVOKABLE void setFullScreen(bool fullScreen);

    bool isDarkTheme() const;
    Q_INVOKABLE void setDarkTheme(bool dark);

    //! The parts (excerpts) defined in the score; the full score is not in the list
    QStringList partNames() const;
    //! Index in partNames(), -1: the full score
    int currentPartIndex() const;
    Q_INVOKABLE void selectPart(int index);

    QString scoreTitle() const;
    bool isPlaying() const;
    bool isPlayAllowed() const;
    bool isMetronomeEnabled() const;
    int measureNumber() const;
    int beatNumber() const;

    Q_INVOKABLE void togglePlay();
    Q_INVOKABLE void rewindToStart();
    Q_INVOKABLE void toggleMetronome();

    //! Shows the file dialog of the project module
    Q_INVOKABLE void openScore();
    //! Opens the given file (drag & drop)
    Q_INVOKABLE void openScoreUrl(const QUrl& url);
    Q_INVOKABLE bool isUrlSupported(const QUrl& url) const;
    Q_INVOKABLE void closeScore();

    //! Opens the score file of a session; if the file is gone, asks to delete the session
    //! or to link it to the file at its new location
    Q_INVOKABLE void openSession(const QString& sessionId);
    Q_INVOKABLE void removeSession(const QString& sessionId);

    Q_INVOKABLE void showAbout();
    Q_INVOKABLE void quit();

signals:
    void hasProjectChanged();
    void isContinuousViewChanged();
    void isPresentationWindowOpenChanged();
    void isTracksPanelVisibleChanged();
    void isFullScreenChanged();
    void isDarkThemeChanged();
    void scoreTitleChanged();
    void partsChanged();
    void currentPartChanged();
    void isPlayingChanged();
    void isPlayAllowedChanged();
    void isMetronomeEnabledChanged();
    void playPositionChanged();

private:
    notation::ViewMode sessionViewMode() const;

    //! Creates or finds the stored session of the open score file
    void activateSession();
    void restorePlayhead();
    void restorePart();
    void subscribeToParts();
    void askForMissingFile(const QString& sessionId);
    void relinkSession(const QString& sessionId);
    void savePlayhead();
};
}
