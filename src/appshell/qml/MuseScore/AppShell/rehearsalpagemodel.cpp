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
#include <QJsonArray>
#include <QJsonObject>
#include <QTimer>

#include "actions/actiontypes.h"
#include "translation.h"
#include "async/notification.h"

#include "notation/inotation.h"
#include "notation/imasternotation.h"
#include "notation/iexcerptnotation.h"
#include "project/inotationproject.h"
#include "project/projectcommands.h"
#include "playback/playbackcommands.h"
#include "ui/uitypes.h"

#include "appshellcommands.h"
#include "rehearsalsessionstore.h"
#include "webbridge.h"

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
    QString partName; // empty: the full score
};

//! The laser pointer tool (otherwise the hand) and its mode: 0 = rectangular selection (frame), 1 = pen, 2 = select
//! and edit annotations. Session-wide as well.
int& laserModeValue()
{
    static int s_mode = 0;
    return s_mode;
}

bool& chordScrubFlag()
{
    static bool s_active = false;
    return s_active;
}

bool& pitchPipeFlag()
{
    static bool s_active = false;
    return s_active;
}

bool& laserPointerFlag()
{
    static bool s_active = false;
    return s_active;
}

//! The presentation window is a session-wide state as well (page content and floating bar share it)
bool& presentationWindowOpenFlag()
{
    static bool s_open = false;
    return s_open;
}

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
    session.partName = stored.partName;
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
        stored->partName = state.partName;
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
        emit isPresentationWindowOpenChanged();
        emit isLaserPointerActiveChanged();
        emit isPitchPipeActiveChanged();
        emit isChordScrubActiveChanged();
        emit laserModeChanged();
        emit annotationsChanged();
        emit isTracksPanelVisibleChanged();
    });

    mainWindow()->isFullScreenChanged().onNotify(this, [this]() {
        emit isFullScreenChanged();
    });

    uiConfiguration()->currentThemeChanged().onNotify(this, [this]() {
        emit isDarkThemeChanged();
    });

    globalContext()->currentProjectChanged().onNotify(this, [this]() {
        //! The tab of the presentation in the browser shows the score it was opened for
        if (web::isWeb()) {
            setIsPresentationWindowOpen(false);
        }

        if (auto project = globalContext()->currentProject()) {
            project->displayNameChanged().onNotify(this, [this]() {
                emit scoreTitleChanged();
            });

            activateSession();
            subscribeToParts();
            restorePart();
        } else {
            //! The window shows the score of the project: it closes with it
            setIsPresentationWindowOpen(false);
        }

        emit hasProjectChanged();
        emit isContinuousViewChanged();
        emit isTracksPanelVisibleChanged();
        emit scoreTitleChanged();
        emit partsChanged();
        emit currentPartChanged();
        emit annotationsChanged();
    });

    globalContext()->currentNotationChanged().onNotify(this, [this]() {
        emit scoreTitleChanged();
        emit currentPartChanged();
        emit annotationsChanged();
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
        subscribeToParts();
        restorePart();
        restorePlayhead();
    }
}

void RehearsalPageModel::subscribeToParts()
{
    const IMasterNotationPtr master = globalContext()->currentMasterNotation();
    if (!master) {
        return;
    }

    master->excerptsChanged().onNotify(this, [this]() {
        emit partsChanged();
        emit currentPartChanged();
        emit annotationsChanged();
    });
}

void RehearsalPageModel::restorePart()
{
    //! The part selected the last time; a part that no longer exists means the full score
    const RehearsalViewSession* session = sessionFor(globalContext()->currentProject());
    if (!session || session->partName.isEmpty()) {
        return;
    }

    const QStringList names = partNames();
    selectPart(static_cast<int>(names.indexOf(session->partName)));
}

QStringList RehearsalPageModel::partNames() const
{
    QStringList names;
    if (const IMasterNotationPtr master = globalContext()->currentMasterNotation()) {
        for (const IExcerptNotationPtr& excerpt : master->excerpts()) {
            names << excerpt->name();
        }
    }
    return names;
}

int RehearsalPageModel::currentPartIndex() const
{
    const IMasterNotationPtr master = globalContext()->currentMasterNotation();
    const INotationPtr current = globalContext()->currentNotation();
    if (!master || !current) {
        return -1;
    }

    const ExcerptNotationList& excerpts = master->excerpts();
    for (size_t i = 0; i < excerpts.size(); ++i) {
        if (excerpts[i]->notation() == current) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void RehearsalPageModel::selectPart(int index)
{
    const IMasterNotationPtr master = globalContext()->currentMasterNotation();
    if (!master) {
        return;
    }

    //! The playback always runs on the full score, so the playhead stays where it is: both views
    //! just show another notation of it
    const ExcerptNotationList& excerpts = master->excerpts();
    if (index >= 0 && index < static_cast<int>(excerpts.size())) {
        const INotationPtr notation = excerpts[index]->notation();
        master->setExcerptIsOpen(notation, true);
        globalContext()->setCurrentNotation(notation);
    } else {
        index = -1;
        globalContext()->setCurrentNotation(master->notation());
    }

    if (RehearsalViewSession* session = sessionFor(globalContext()->currentProject())) {
        const QString name = index >= 0 ? excerpts[index]->name() : QString();
        if (session->partName != name) {
            session->partName = name;
            storeSessionState(*session);
        }
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
    return presentationWindowOpenFlag() || sessionViewMode() == ViewMode::LINE;
}

bool RehearsalPageModel::isPresentationWindowOpen() const
{
    return presentationWindowOpenFlag();
}

void RehearsalPageModel::setIsPresentationWindowOpen(bool open)
{
    if (presentationWindowOpenFlag() == open) {
        return;
    }

    //! In the browser the presentation is a tab of its own (it is opened by the click that gets here)
    if (web::isWeb()) {
        if (open && !web::openPresentationTab(scorePath())) {
            return;
        }

        if (!open) {
            web::closePresentationTab();
        }
    }

    presentationWindowOpenFlag() = open;

    sessionChangedNotification().notify();
}

void RehearsalPageModel::presentationTabClosed()
{
    if (!presentationWindowOpenFlag()) {
        return;
    }

    presentationWindowOpenFlag() = false;

    sessionChangedNotification().notify();
}

void RehearsalPageModel::setIsContinuousView(bool continuous)
{
    RehearsalViewSession* session = sessionFor(globalContext()->currentProject());
    const ViewMode mode = continuous ? ViewMode::LINE : ViewMode::PAGE;
    if (!session || session->viewMode == mode || (presentationWindowOpenFlag() && !continuous)) {
        return;
    }

    session->viewMode = mode;
    storeSessionState(*session);
    sessionChangedNotification().notify();
}

int RehearsalPageModel::viewModeOverride() const
{
    return static_cast<int>(presentationWindowOpenFlag() ? ViewMode::LINE : sessionViewMode());
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

double RehearsalPageModel::playbackSecs() const
{
    return globalContext()->playbackState()->playbackPosition();
}

QString RehearsalPageModel::scorePath() const
{
    const INotationProjectPtr project = globalContext()->currentProject();
    return project ? project->path().toQString() : QString();
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
            //! NOTE The next dialog can only be opened after this one is closed
            QTimer::singleShot(0, this, [this, sessionId]() {
                relinkSession(sessionId);
            });
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

bool RehearsalPageModel::isLaserPointerActive() const
{
    return laserPointerFlag();
}

void RehearsalPageModel::setIsLaserPointerActive(bool active)
{
    if (laserPointerFlag() == active) {
        return;
    }

    laserPointerFlag() = active;
    if (active) {
        pitchPipeFlag() = false;
        chordScrubFlag() = false;
    }
    sessionChangedNotification().notify();
}

bool RehearsalPageModel::isPitchPipeActive() const
{
    return pitchPipeFlag();
}

void RehearsalPageModel::setIsPitchPipeActive(bool active)
{
    if (pitchPipeFlag() == active) {
        return;
    }

    pitchPipeFlag() = active;
    if (active) {
        laserPointerFlag() = false;
        chordScrubFlag() = false;
    }
    sessionChangedNotification().notify();
}

bool RehearsalPageModel::isChordScrubActive() const
{
    return chordScrubFlag();
}

void RehearsalPageModel::setIsChordScrubActive(bool active)
{
    if (chordScrubFlag() == active) {
        return;
    }

    chordScrubFlag() = active;
    if (active) {
        laserPointerFlag() = false;
        pitchPipeFlag() = false;
    }
    sessionChangedNotification().notify();
}

int RehearsalPageModel::laserMode() const
{
    return laserModeValue();
}

void RehearsalPageModel::setLaserMode(int mode)
{
    mode = std::clamp(mode, 0, 2);
    if (laserModeValue() == mode) {
        return;
    }

    laserModeValue() = mode;
    sessionChangedNotification().notify();
}

namespace {
QString annotationKey(const RehearsalViewSession& state)
{
    return state.partName + (state.viewMode == ViewMode::LINE ? "|continuous" : "|page");
}

//! The stored session of the current project, with the key of the current part / view mode
RehearsalSession* storedSessionOf(const INotationProjectPtr& project, QString& key)
{
    RehearsalViewSession* state = sessionFor(project);
    if (!state) {
        return nullptr;
    }

    key = annotationKey(*state);
    return RehearsalSessionStore::instance().findByPath(project->path().toQString());
}

QJsonObject currentAnnotations(const INotationProjectPtr& project)
{
    QString key;
    const RehearsalSession* stored = storedSessionOf(project, key);
    return stored ? stored->annotations.value(key).toObject() : QJsonObject();
}

void storeAnnotations(const INotationProjectPtr& project, const QJsonObject& annotations)
{
    QString key;
    RehearsalSession* stored = storedSessionOf(project, key);
    if (!stored) {
        return;
    }

    if (annotations.isEmpty()) {
        stored->annotations.remove(key);
    } else {
        stored->annotations.insert(key, annotations);
    }

    RehearsalSessionStore::instance().markChanged(stored->id);
    sessionChangedNotification().notify();
}
}

QVariantList RehearsalPageModel::strokes() const
{
    QVariantList result;
    const QJsonArray strokes = currentAnnotations(globalContext()->currentProject()).value("strokes").toArray();
    for (const QJsonValue& strokeValue : strokes) {
        const QJsonArray numbers = strokeValue.toArray();
        QVariantList points;
        for (int i = 0; i + 1 < numbers.size(); i += 2) {
            points.append(QPointF(numbers.at(i).toDouble(), numbers.at(i + 1).toDouble()));
        }
        result.append(QVariant(points));
    }
    return result;
}

bool RehearsalPageModel::hasHighlight() const
{
    return currentAnnotations(globalContext()->currentProject()).value("highlight").toArray().size() == 4;
}

QRectF RehearsalPageModel::highlight() const
{
    const QJsonArray a = currentAnnotations(globalContext()->currentProject()).value("highlight").toArray();
    return a.size() == 4 ? QRectF(a.at(0).toDouble(), a.at(1).toDouble(), a.at(2).toDouble(), a.at(3).toDouble()) : QRectF();
}

void RehearsalPageModel::addStroke(const QVariantList& points)
{
    const INotationProjectPtr project = globalContext()->currentProject();
    QJsonArray numbers;
    for (const QVariant& point : points) {
        const QPointF p = point.toPointF();
        numbers.append(std::round(p.x() * 10.0) / 10.0);
        numbers.append(std::round(p.y() * 10.0) / 10.0);
    }

    if (!project || numbers.size() < 4) {
        return;
    }

    QJsonObject annotations = currentAnnotations(project);
    QJsonArray strokes = annotations.value("strokes").toArray();
    strokes.append(numbers);
    annotations["strokes"] = strokes;
    storeAnnotations(project, annotations);
}

void RehearsalPageModel::setStroke(int index, const QVariantList& points)
{
    const INotationProjectPtr project = globalContext()->currentProject();
    QJsonObject annotations = currentAnnotations(project);
    QJsonArray strokes = annotations.value("strokes").toArray();
    if (!project || index < 0 || index >= strokes.size() || points.size() < 2) {
        return;
    }

    QJsonArray numbers;
    for (const QVariant& point : points) {
        const QPointF p = point.toPointF();
        numbers.append(std::round(p.x() * 10.0) / 10.0);
        numbers.append(std::round(p.y() * 10.0) / 10.0);
    }

    strokes[index] = numbers;
    annotations["strokes"] = strokes;
    storeAnnotations(project, annotations);
}

void RehearsalPageModel::removeStroke(int index)
{
    const INotationProjectPtr project = globalContext()->currentProject();
    QJsonObject annotations = currentAnnotations(project);
    QJsonArray strokes = annotations.value("strokes").toArray();
    if (!project || index < 0 || index >= strokes.size()) {
        return;
    }

    strokes.removeAt(index);
    if (strokes.isEmpty()) {
        annotations.remove("strokes");
    } else {
        annotations["strokes"] = strokes;
    }

    storeAnnotations(project, annotations);
}

void RehearsalPageModel::setHighlight(const QRectF& rect)
{
    const INotationProjectPtr project = globalContext()->currentProject();
    if (!project) {
        return;
    }

    QJsonObject annotations = currentAnnotations(project);
    annotations["highlight"] = QJsonArray { rect.x(), rect.y(), rect.width(), rect.height() };
    storeAnnotations(project, annotations);
}

void RehearsalPageModel::clearHighlight()
{
    const INotationProjectPtr project = globalContext()->currentProject();
    QJsonObject annotations = currentAnnotations(project);
    if (!project || !annotations.contains("highlight")) {
        return;
    }

    annotations.remove("highlight");
    storeAnnotations(project, annotations);
}

void RehearsalPageModel::clearAnnotations()
{
    if (const INotationProjectPtr project = globalContext()->currentProject()) {
        storeAnnotations(project, QJsonObject());
    }
}

void RehearsalPageModel::askClearAnnotations()
{
    interactive()->question(muse::trc("appshell", "Delete annotations"),
                            muse::trc("appshell", "Do you really want to delete all annotations?"),
                            { IInteractive::Button::No, IInteractive::Button::Yes },
                            IInteractive::Button::No)
    .onResolve(this, [this](const IInteractive::Result& res) {
        if (res.isButton(IInteractive::Button::Yes)) {
            clearAnnotations();
        }
    });
}
