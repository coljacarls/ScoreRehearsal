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

#include "rehearsalpagemodel.h"

#include <algorithm>
#include <cmath>

#include <QDir>
#include <QFileInfo>

#include "actions/actiontypes.h"
#include "translation.h"
#include "async/notification.h"

#include "notation/inotation.h"
#include "project/inotationproject.h"
#include "project/projectcommands.h"
#include "playback/playbackcommands.h"
#include "ui/uitypes.h"

#include "appshellcommands.h"
#include "rehearsalsessionstore.h"

using namespace mu::appshell;
using namespace mu::notation;
using namespace mu::project;
using namespace muse;

namespace {
//! Lives as long as the project is open, independent of the page (and this model) being loaded
struct RehearsalViewSession {
    std::weak_ptr<INotationProject> project;
    ViewMode viewMode = ViewMode::LINE; // continuous view by default
    bool tracksPanelVisible = true;
};

//! Notifies all live model instances (page content, floating bar)
//! that the session state has changed
muse::async::Notification& sessionChangedNotification()
{
    static muse::async::Notification s_notification;
    return s_notification;
}

RehearsalViewSession* sessionFor(const INotationProjectPtr& project)
{
    static std::vector<RehearsalViewSession> s_sessions;

    std::erase_if(s_sessions, [](const RehearsalViewSession& session) {
        return session.project.expired();
    });

    if (!project) {
        return nullptr;
    }

    for (RehearsalViewSession& session : s_sessions) {
        if (session.project.lock() == project) {
            return &session;
        }
    }

    RehearsalViewSession& session = s_sessions.emplace_back();
    session.project = project;

    //! Start from what was stored for the file the last time
    RehearsalSessionStore& store = RehearsalSessionStore::instance();
    const RehearsalSession& stored = store.ensureForPath(project->path().toQString(), project->displayName());
    session.viewMode = stored.continuousView ? ViewMode::LINE : ViewMode::PAGE;
    session.tracksPanelVisible = stored.tracksPanelVisible;
    return &session;
}
}

namespace {
//! The playhead is restored once per opened project, before that it must not be overwritten
std::weak_ptr<INotationProject>& playheadRestoredProject()
{
    static std::weak_ptr<INotationProject> s_project;
    return s_project;
}

void storeSessionState(const RehearsalViewSession& state)
{
    RehearsalSessionStore& store = RehearsalSessionStore::instance();
    const INotationProjectPtr project = state.project.lock();
    if (!project) {
        return;
    }

    if (RehearsalSession* stored = store.findByPath(project->path().toQString())) {
        stored->continuousView = state.viewMode == ViewMode::LINE;
        stored->tracksPanelVisible = state.tracksPanelVisible;
        store.markChanged(stored->id);
    }
}
}

RehearsalPageModel::RehearsalPageModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

void RehearsalPageModel::load()
{
    sessionChangedNotification().onNotify(this, [this]() {
        emit isContinuousViewChanged();
        emit isTracksPanelVisibleChanged();
    });

    mainWindow()->isFullScreenChanged().onNotify(this, [this]() {
        emit isFullScreenChanged();
    });

    uiConfiguration()->currentThemeChanged().onNotify(this, [this]() {
        emit isDarkThemeChanged();
    });

    globalContext()->currentProjectChanged().onNotify(this, [this]() {
        if (auto project = globalContext()->currentProject()) {
            project->displayNameChanged().onNotify(this, [this]() {
                emit scoreTitleChanged();
            });

            activateSession();
        }

        emit hasProjectChanged();
        emit isContinuousViewChanged();
        emit isTracksPanelVisibleChanged();
        emit scoreTitleChanged();
    });

    globalContext()->currentNotationChanged().onNotify(this, [this]() {
        emit scoreTitleChanged();
    });

    globalContext()->playbackState()->playbackStatusChanged().onReceive(this, [this](audio::PlaybackStatus) {
        emit isPlayingChanged();
    });

    globalContext()->playbackState()->playbackPositionChanged().onReceive(this, [this](audio::secs_t) {
        emit playPositionChanged();
        savePlayhead();
    });

    playbackController()->playbackInitedChanged().onReceive(this, [this](bool) {
        restorePlayhead();
    });

    playbackController()->isPlayAllowedChanged().onReceive(this, [this](bool) {
        emit isPlayAllowedChanged();
    });

    notationConfiguration()->isMetronomeEnabledChanged().onNotify(this, [this]() {
        emit isMetronomeEnabledChanged();
    });

    if (auto project = globalContext()->currentProject()) {
        project->displayNameChanged().onNotify(this, [this]() {
            emit scoreTitleChanged();
        });

        activateSession();
        restorePlayhead();
    }
}

void RehearsalPageModel::activateSession()
{
    const INotationProjectPtr project = globalContext()->currentProject();
    if (!project) {
        return;
    }

    //! The first score file opened creates the session; reopening brings it to the top of the list
    RehearsalSessionStore& store = RehearsalSessionStore::instance();
    const RehearsalSession& session = store.ensureForPath(project->path().toQString(), project->displayName());
    store.setTitle(session.id, project->displayName());
    store.touch(session.id);
}

void RehearsalPageModel::restorePlayhead()
{
    const INotationProjectPtr project = globalContext()->currentProject();
    if (!project || !playbackController()->isPlaybackInited() || playheadRestoredProject().lock() == project) {
        return;
    }

    playheadRestoredProject() = project;

    const RehearsalSession* session = RehearsalSessionStore::instance().findByPath(project->path().toQString());
    if (!session || session->playheadMeasureIndex <= 0) {
        return;
    }

    //! NOTE The score may have changed since: a measure number beyond the end is moved to the last measure
    const int lastMeasure = std::max(0, playbackController()->currentBeat().maxMeasureIndex);
    playbackController()->seekBeat(std::min(session->playheadMeasureIndex, lastMeasure), 0, false);
}

void RehearsalPageModel::savePlayhead()
{
    const INotationProjectPtr project = globalContext()->currentProject();
    if (!project || playheadRestoredProject().lock() != project) {
        return;
    }

    RehearsalSessionStore& store = RehearsalSessionStore::instance();
    RehearsalSession* session = store.findByPath(project->path().toQString());
    const int measureIndex = playbackController()->currentBeat().measureIndex;
    if (!session || session->playheadMeasureIndex == measureIndex) {
        return;
    }

    session->playheadMeasureIndex = measureIndex;
    store.markChanged(session->id);
}

bool RehearsalPageModel::hasProject() const
{
    return globalContext()->currentProject() != nullptr;
}

ViewMode RehearsalPageModel::sessionViewMode() const
{
    const RehearsalViewSession* session = sessionFor(globalContext()->currentProject());
    return session ? session->viewMode : ViewMode::LINE;
}

bool RehearsalPageModel::isContinuousView() const
{
    return sessionViewMode() == ViewMode::LINE;
}

void RehearsalPageModel::setIsContinuousView(bool continuous)
{
    RehearsalViewSession* session = sessionFor(globalContext()->currentProject());
    const ViewMode mode = continuous ? ViewMode::LINE : ViewMode::PAGE;
    if (!session || session->viewMode == mode) {
        return;
    }

    session->viewMode = mode;
    storeSessionState(*session);
    sessionChangedNotification().notify();
}

int RehearsalPageModel::viewModeOverride() const
{
    return static_cast<int>(sessionViewMode());
}

bool RehearsalPageModel::isTracksPanelVisible() const
{
    const RehearsalViewSession* session = sessionFor(globalContext()->currentProject());
    return session ? session->tracksPanelVisible : true;
}

void RehearsalPageModel::setIsTracksPanelVisible(bool visible)
{
    RehearsalViewSession* session = sessionFor(globalContext()->currentProject());
    if (!session || session->tracksPanelVisible == visible) {
        return;
    }

    session->tracksPanelVisible = visible;
    storeSessionState(*session);
    sessionChangedNotification().notify();
}

bool RehearsalPageModel::isFullScreen() const
{
    return mainWindow()->isFullScreen();
}

void RehearsalPageModel::setFullScreen(bool fullScreen)
{
    if (mainWindow()->isFullScreen() == fullScreen) {
        return;
    }

    mainWindow()->toggleFullScreen();
}

bool RehearsalPageModel::isDarkTheme() const
{
    return uiConfiguration()->isDarkMode();
}

void RehearsalPageModel::setDarkTheme(bool dark)
{
    uiConfiguration()->setCurrentTheme(dark ? muse::ui::DARK_THEME_CODE : muse::ui::LIGHT_THEME_CODE);
}

QString RehearsalPageModel::scoreTitle() const
{
    INotationPtr notation = globalContext()->currentNotation();
    return notation ? notation->projectNameAndPartName() : QString();
}

bool RehearsalPageModel::isPlaying() const
{
    return globalContext()->playbackState()->isPlaying();
}

bool RehearsalPageModel::isPlayAllowed() const
{
    return playbackController()->isPlayAllowed();
}

bool RehearsalPageModel::isMetronomeEnabled() const
{
    return notationConfiguration()->isMetronomeEnabled();
}

int RehearsalPageModel::measureNumber() const
{
    return playbackController()->currentBeat().measureIndex + 1;
}

int RehearsalPageModel::beatNumber() const
{
    return static_cast<int>(std::floor(playbackController()->currentBeat().beat)) + 1;
}

void RehearsalPageModel::togglePlay()
{
    dispatcher()->dispatch(playback::PLAY_TOGGLE_COMMAND);
}

void RehearsalPageModel::rewindToStart()
{
    dispatcher()->dispatch(playback::REWIND_COMMAND, { { "position", Val(0.0) } });
}

void RehearsalPageModel::toggleMetronome()
{
    dispatcher()->dispatch(playback::METRONOME_TOGGLE_COMMAND);
}

void RehearsalPageModel::openScore()
{
    //! NOTE Without a url the project module shows its file dialog
    dispatcher()->dispatch(project::PROJECT_OPEN_COMMAND);
}

void RehearsalPageModel::openScoreUrl(const QUrl& url)
{
    actionsDispatcher()->dispatch("file-open", actions::ActionData::make_arg1<QUrl>(url));
}

bool RehearsalPageModel::isUrlSupported(const QUrl& url) const
{
    return openProjectScenario()->isUrlSupported(url);
}

void RehearsalPageModel::closeScore()
{
    dispatcher()->dispatch(project::PROJECT_CLOSE_COMMAND);
}

void RehearsalPageModel::openSession(const QString& sessionId)
{
    const RehearsalSession* session = RehearsalSessionStore::instance().findById(sessionId);
    if (!session) {
        return;
    }

    if (!QFileInfo::exists(session->filePath)) {
        askForMissingFile(sessionId);
        return;
    }

    openScoreUrl(QUrl::fromLocalFile(session->filePath));
}

void RehearsalPageModel::removeSession(const QString& sessionId)
{
    RehearsalSessionStore::instance().remove(sessionId);
}

void RehearsalPageModel::askForMissingFile(const QString& sessionId)
{
    const RehearsalSession* session = RehearsalSessionStore::instance().findById(sessionId);
    if (!session) {
        return;
    }

    const int deleteBtn = int(IInteractive::Button::CustomButton) + 1;
    const int relinkBtn = int(IInteractive::Button::CustomButton) + 2;

    const std::string title = muse::trc("appshell", "The score file cannot be found");
    const std::string text = muse::qtrc("appshell", "The file of the session \"%1\" was moved or deleted:\n%2")
                             .arg(session->title, QDir::toNativeSeparators(session->filePath)).toStdString();

    IInteractive::ButtonData cancelBtn = interactive()->buttonData(IInteractive::Button::Cancel);

    interactive()->warning(title, text,
                           { cancelBtn,
                             IInteractive::ButtonData(deleteBtn, muse::trc("appshell", "Delete session")),
                             IInteractive::ButtonData(relinkBtn, muse::trc("appshell", "Locate file…"), true) },
                           relinkBtn, { IInteractive::Option::WithIcon })
    .onResolve(this, [this, sessionId, deleteBtn, relinkBtn](const IInteractive::Result& res) {
        if (res.isButton(deleteBtn)) {
            removeSession(sessionId);
        } else if (res.isButton(relinkBtn)) {
            relinkSession(sessionId);
        }
    });
}

void RehearsalPageModel::relinkSession(const QString& sessionId)
{
    const RehearsalSession* session = RehearsalSessionStore::instance().findById(sessionId);
    if (!session) {
        return;
    }

    const QFileInfo oldFile(session->filePath);
    const muse::io::path_t startDir = QFileInfo::exists(oldFile.absolutePath()) ? oldFile.absolutePath() : QDir::homePath();

    const std::vector<std::string> filter { muse::trc("appshell", "Score files") + " (*.mscz *.mscx)",
                                            muse::trc("appshell", "All files") + " (*)" };

    interactive()->selectOpeningFile(muse::trc("appshell", "Locate score file"), startDir, filter)
    .onResolve(this, [this, sessionId](const muse::io::path_t& path) {
        if (path.empty()) {
            return;
        }

        RehearsalSessionStore::instance().relink(sessionId, path.toQString());
        openSession(sessionId);
    });
}

void RehearsalPageModel::showAbout()
{
    dispatcher()->dispatch(APP_ABOUT_COMMAND);
}

void RehearsalPageModel::quit()
{
    dispatcher()->dispatch(APP_QUIT_COMMAND);
}
