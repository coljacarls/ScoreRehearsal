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

#include <memory>

#include <QColor>
#include <QElapsedTimer>
#include <QQuickWindow>
#include <QRectF>
#include <qqmlintegration.h>

#include "async/asyncable.h"
#include "context/iglobalcontext.h"
#include "draw/types/geometry.h"
#include "modularity/ioc.h"
#include "notation/inotationconfiguration.h"
#include "notationscene/qml/MuseScore/NotationScene/abstractnotationpaintview.h"


namespace mu::notation {
class PlaybackCursor;
}

namespace mu::appshell {
//! Score view of the presentation window: shows the whole score zoomed to the window height,
//! with the playhead fixed near the left edge. The score scrolls underneath it (karaoke style).
//! It is independent of the rehearsal view: panning or zooming there has no effect here,
//! only the playback position (seeking and playing) is followed.
//!
//! While playing, the position is advanced once per displayed frame (not by the timer of the
//! base class) from a smoothed playback clock, and the playhead is an item of its own at a
//! fixed place: both keep the movement even.
class PresentationPaintView : public notation::AbstractNotationPaintView
{
    Q_OBJECT

    Q_PROPERTY(QRectF playheadRect READ playheadRect NOTIFY playheadRectChanged)
    Q_PROPERTY(QColor playheadColor READ playheadColor CONSTANT)

    QML_ELEMENT

    muse::GlobalInject<notation::INotationConfiguration> notationConfig;
    muse::ContextInject<context::IGlobalContext> globalContext = { this };

public:
    explicit PresentationPaintView(QQuickItem* parent = nullptr);
    ~PresentationPaintView() override;

    //! In view coordinates; empty while the position is unknown
    QRectF playheadRect() const;
    QColor playheadColor() const;

signals:
    void playheadRectChanged();

private slots:
    void onWindowChanged(QQuickWindow* window);
    void onFrame();

private:
    bool followPlaybackCursor(const muse::RectF& cursorRect) override;
    void onViewSizeChanged() override;
    void onLoadNotation(notation::INotationPtr notation) override;
    void onUnloadNotation(notation::INotationPtr notation) override;
    void paint(QPainter* painter) override;

    //! The view does not use the zoom and position stored in the (shared) view state
    void initZoomAndPosition() override;

    //! Scales the score to the window height and puts the cursor at its fixed place
    void alignToCursor(const muse::RectF& cursorRect);
    void updatePlayhead(const muse::RectF& cursorRect);

    //! Width of about one measure (in logical units)
    qreal measureWidth() const;

    void onPlaybackPositionChanged(muse::secs_t secs);
    void requestFrame();
    void logStats(qint64 paintNs);

    std::unique_ptr<notation::PlaybackCursor> m_cursor;
    muse::async::Asyncable m_subscriptions;

    muse::RectF m_lastCursorRect;
    QRectF m_playheadRect;

    //! The playback clock: the last reported position, and the position that is shown
    QElapsedTimer m_clock;
    qint64 m_reportedAtNs = 0;
    qint64 m_lastFrameNs = 0;
    muse::secs_t m_reportedSecs = 0.0;
    muse::secs_t m_shownSecs = 0.0;
    bool m_clockRunning = false;

    //! Measurement of the time spent painting (SCOREREHEARSAL_PRESENTATION_STATS=1)
    bool m_statsEnabled = false;
    qint64 m_statsSinceNs = 0;
    qint64 m_statsPaintTotalNs = 0;
    qint64 m_statsPaintMaxNs = 0;
    qint64 m_statsFrameMaxNs = 0;
    int m_statsFrames = 0;
};
}
