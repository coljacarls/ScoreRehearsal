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
#include <QRectF>
#include <QVariantList>
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
    Q_PROPERTY(bool isLaserPointerActive READ isLaserPointerActive WRITE setIsLaserPointerActive
               NOTIFY isLaserPointerActiveChanged)
    Q_PROPERTY(bool isPitchPipeActive READ isPitchPipeActive WRITE setIsPitchPipeActive NOTIFY isPitchPipeActiveChanged)
    Q_PROPERTY(bool isChordScrubActive READ isChordScrubActive WRITE setIsChordScrubActive
               NOTIFY isChordScrubActiveChanged)
    Q_PROPERTY(int laserMode READ laserMode WRITE setLaserMode NOTIFY laserModeChanged)
    Q_PROPERTY(QVariantList strokes READ strokes NOTIFY annotationsChanged)
    Q_PROPERTY(bool hasHighlight READ hasHighlight NOTIFY annotationsChanged)
    Q_PROPERTY(QRectF highlight READ highlight NOTIFY annotationsChanged)
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
    //! The position of the playback in seconds (the presentation tab of the browser version follows it)
    Q_PROPERTY(double playbackSecs READ playbackSecs NOTIFY playPositionChanged)
    //! The file of the score that is open (empty without a score)
    Q_PROPERTY(QString scorePath READ scorePath NOTIFY scoreTitleChanged)

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
    //! The browser version: the user closed the tab of the presentation
    Q_INVOKABLE void presentationTabClosed();

    //! Laser pointer tool (instead of the hand): the score is not dragged, taps do not seek.
    bool isLaserPointerActive() const;
    void setIsLaserPointerActive(bool active);
    //! Chord scrubbing tool (instead of the hand): the score is not dragged, the chord at the finger sounds while it is
    //! held and follows it, without moving the playhead. Excludes the other tools.
    bool isChordScrubActive() const;
    void setIsChordScrubActive(bool active);
    //! Pitch pipe tool (instead of the hand): the score is not dragged, a touched note sounds while it is held.
    //! Excludes the other tools.
    bool isPitchPipeActive() const;
    void setIsPitchPipeActive(bool active);
    //! 0 = rectangular selection (a tap frames the tapped element), 1 = pen (freeform drawing),
    //! 2 = select annotations to resize or delete them
    int laserMode() const;
    void setLaserMode(int mode);

    //! The annotations of the laser pointer on the current part / view mode (logical score coordinates),
    //! stored in the session of the score: freeform strokes (lists of points) and one highlighted rectangle
    QVariantList strokes() const;
    bool hasHighlight() const;
    QRectF highlight() const;
    Q_INVOKABLE void addStroke(const QVariantList& points);
    Q_INVOKABLE void removeStroke(int index);
    //! Replaces the points of a stroke (moved or resized)
    Q_INVOKABLE void setStroke(int index, const QVariantList& points);
    Q_INVOKABLE void setHighlight(const QRectF& rect);
    Q_INVOKABLE void clearHighlight();
    //! Asks whether to delete all annotations, deletes them on yes
    Q_INVOKABLE void askClearAnnotations();

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

    double playbackSecs() const;
    QString scorePath() const;

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
    void isLaserPointerActiveChanged();
    void isPitchPipeActiveChanged();
    void isChordScrubActiveChanged();
    void laserModeChanged();
    void annotationsChanged();
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
    void clearAnnotations();
};
}
