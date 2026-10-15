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

#include "presentationpaintview.h"

#include <algorithm>
#include <cmath>

#include <QPainter>
#include <QQuickWindow>
#include <QtGlobal>

#include "engraving/dom/score.h"
#include "engraving/dom/system.h"
#include "notation/inotationelements.h"
#include "notation/inotationplayback.h"
#include "notation/types/viewmode.h"
#include "notationscene/qml/MuseScore/NotationScene/playbackcursor.h"

#include "log.h"

using namespace mu::appshell;
using namespace mu::notation;
using namespace muse;

namespace {
//! Space above and below the staves, relative to the height of the system
constexpr qreal VERTICAL_MARGIN_RATIO = 0.06;

//! The gap in front of the playhead, relative to the width of the view
constexpr qreal MIN_GAP_RATIO = 0.10;
constexpr qreal MAX_GAP_RATIO = 0.35;

//! A reported position that is further away from the shown one than this is a jump (seek, loop)
constexpr double JUMP_THRESHOLD_SECS = 0.3;

//! Share of the difference between the reported and the shown position that is corrected per frame
constexpr double CLOCK_CORRECTION = 0.15;

//! Frames after a stall are not allowed to move the score by more than this
constexpr double MAX_FRAME_SECS = 0.1;
}

PresentationPaintView::PresentationPaintView(QQuickItem* parent)
    : AbstractNotationPaintView(parent)
{
    setReadonly(true);
    setIsMainView(false);
    setAcceptedMouseButtons(Qt::NoButton);

    //! The presentation needs the continuous view (a single system)
    setViewModeOverride(static_cast<int>(ViewMode::LINE));

    //! While stopped, the cursor still has to follow the position set in the rehearsal view
    setTapToSeek(true);

    m_clock.start();
    m_statsEnabled = qEnvironmentVariableIntValue("SCOREREHEARSAL_PRESENTATION_STATS") != 0;

    connect(this, &QQuickItem::windowChanged, this, &PresentationPaintView::onWindowChanged);
}

PresentationPaintView::~PresentationPaintView() = default;

QRectF PresentationPaintView::playheadRect() const
{
    return m_playheadRect;
}

QColor PresentationPaintView::playheadColor() const
{
    return notationConfig()->playbackCursorColor();
}

void PresentationPaintView::initZoomAndPosition()
{
}

void PresentationPaintView::onLoadNotation(INotationPtr notation)
{
    AbstractNotationPaintView::onLoadNotation(notation);

    //! The playback cursor of the base class is driven by its own timer, which is not in step with
    //! the display: this view resolves the cursor position itself, once per frame.
    //! NOTE: not in the constructor, the IoC context of a QML object is not available yet there
    if (!m_cursor) {
        m_cursor = std::make_unique<PlaybackCursor>(iocContext());
    }

    m_cursor->setNotation(notation);

    //! NOTE: the base class subscribes with SetReplace, so this view keeps its own subscription holder
    globalContext()->playbackState()->playbackPositionChanged().onReceive(&m_subscriptions, [this](secs_t secs) {
        onPlaybackPositionChanged(secs);
    }, async::Asyncable::Mode::SetReplace);

    //! Frames are only rendered on request: without this, nothing would start the frame loop of onFrame()
    //! when the player starts (the position that is shown does not change yet, so nothing is redrawn)
    globalContext()->playbackState()->playbackStatusChanged().onReceive(&m_subscriptions, [this](audio::PlaybackStatus) {
        if (globalContext()->playbackState()->isPlaying()) {
            requestFrame();
        }
    }, async::Asyncable::Mode::SetReplace);
}

void PresentationPaintView::onUnloadNotation(INotationPtr notation)
{
    globalContext()->playbackState()->playbackPositionChanged().disconnect(&m_subscriptions);
    globalContext()->playbackState()->playbackStatusChanged().disconnect(&m_subscriptions);

    if (m_cursor) {
        m_cursor->setNotation(nullptr);
    }

    m_lastCursorRect = RectF();
    m_clockRunning = false;

    if (!m_playheadRect.isNull()) {
        m_playheadRect = QRectF();
        emit playheadRectChanged();
    }

    AbstractNotationPaintView::onUnloadNotation(notation);
}

void PresentationPaintView::onWindowChanged(QQuickWindow* window)
{
    //! Emitted after the animations of a frame were advanced, right before the frame is rendered
    if (window) {
        connect(window, &QQuickWindow::afterAnimating, this, &PresentationPaintView::onFrame, Qt::UniqueConnection);
    }
}

void PresentationPaintView::onPlaybackPositionChanged(secs_t secs)
{
    m_reportedSecs = secs;
    m_reportedAtNs = m_clock.nsecsElapsed();
}

void PresentationPaintView::onFrame()
{
    const qint64 now = m_clock.nsecsElapsed();
    const qint64 sinceLastFrameNs = now - m_lastFrameNs;
    m_lastFrameNs = now;

    if (m_statsEnabled && sinceLastFrameNs < 1000000000) {
        m_statsFrameMaxNs = std::max(m_statsFrameMaxNs, sinceLastFrameNs);
    }

    if (!notation() || !m_cursor || !globalContext()->playbackState()->isPlaying()) {
        m_clockRunning = false;
        return;
    }

    const INotationPlaybackPtr playback = notationPlayback();
    if (!playback) {
        return;
    }

    //! The position reported by the audio engine arrives in steps and with some jitter.
    //! The shown position runs with the (steady) frame time and is only steered towards it.
    const secs_t estimated = m_reportedSecs + (now - m_reportedAtNs) / 1e9;
    if (!m_clockRunning || std::abs(estimated - m_shownSecs) > JUMP_THRESHOLD_SECS) {
        m_shownSecs = estimated;
        m_clockRunning = true;
    } else {
        const double dt = std::min(sinceLastFrameNs / 1e9, MAX_FRAME_SECS);
        m_shownSecs += dt;
        m_shownSecs += (estimated - m_shownSecs) * CLOCK_CORRECTION;
    }

    m_cursor->move(playback->secToTick(m_shownSecs));

    const RectF cursorRect = m_cursor->rect();
    if (!cursorRect.isNull()) {
        m_lastCursorRect = cursorRect;
        alignToCursor(cursorRect);
    }

    //! Keeps the frames coming, also when the canvas could not be moved (e.g. at the end of the score)
    requestFrame();
}

void PresentationPaintView::requestFrame()
{
    //! QQuickWindow::update() always renders another frame (unlike QQuickItem::update(),
    //! which only does when something in the scene changed)
    if (QQuickWindow* w = window()) {
        w->update();
    }
}

void PresentationPaintView::onViewSizeChanged()
{
    if (!notation() || !viewport().isValid()) {
        return;
    }

    if (m_lastCursorRect.isValid()) {
        alignToCursor(m_lastCursorRect);
    }

    scheduleRedraw();
}

bool PresentationPaintView::followPlaybackCursor(const RectF& cursorRect)
{
    //! While playing, onFrame() moves the score; here only the position of a stopped player is followed
    //! (seeking in the rehearsal view)
    if (globalContext()->playbackState()->isPlaying()) {
        return true;
    }

    m_lastCursorRect = cursorRect;
    alignToCursor(cursorRect);
    return true;
}

void PresentationPaintView::alignToCursor(const RectF& cursorRect)
{
    if (!notation() || cursorRect.isNull() || width() <= 0 || height() <= 0) {
        return;
    }

    //! The cursor spans the system: this is the part of the score that has to fit into the window height
    const qreal systemHeight = cursorRect.height() * (1.0 + 2 * VERTICAL_MARGIN_RATIO);
    if (systemHeight > 0) {
        setScaling(height() / systemHeight, PointF());
    }

    const RectF view = viewport();

    //! The playhead sits behind the sticky panel (instrument names, clef) and about one measure
    //! of notes that were already sung
    const qreal gap = std::clamp(measureWidth(), view.width() * MIN_GAP_RATIO, view.width() * MAX_GAP_RATIO);
    const qreal left = cursorRect.left() - continuousPanelWidth() - gap;
    const qreal top = cursorRect.center().y() - view.height() / 2;

    moveCanvasToPosition(PointF(left, top));
    updatePlayhead(cursorRect);
}

void PresentationPaintView::updatePlayhead(const RectF& cursorRect)
{
    QRectF rect = fromLogical(cursorRect).toQRectF();

    //! Whole pixels: a playhead that stands still on the screen does not flicker
    rect.setX(std::round(rect.x()));
    rect.setWidth(std::max(std::round(cursorRect.width() * currentScaling()), 1.0));
    rect = rect.intersected(QRectF(0.0, 0.0, width(), height()));

    if (rect != m_playheadRect) {
        m_playheadRect = rect;
        emit playheadRectChanged();
    }
}

qreal PresentationPaintView::measureWidth() const
{
    const engraving::Score* score = notation()->elements()->msScore();
    if (!score || score->systems().empty() || score->nmeasures() == 0) {
        return 0.0;
    }

    return score->systems().front()->width() / static_cast<qreal>(score->nmeasures());
}

void PresentationPaintView::paint(QPainter* painter)
{
    if (!m_statsEnabled) {
        AbstractNotationPaintView::paint(painter);
        return;
    }

    QElapsedTimer timer;
    timer.start();
    AbstractNotationPaintView::paint(painter);
    logStats(timer.nsecsElapsed());
}

void PresentationPaintView::logStats(qint64 paintNs)
{
    const qint64 now = m_clock.nsecsElapsed();
    ++m_statsFrames;
    m_statsPaintTotalNs += paintNs;
    m_statsPaintMaxNs = std::max(m_statsPaintMaxNs, paintNs);

    if (now - m_statsSinceNs < 1000000000) {
        return;
    }

    LOGI() << "presentation view: " << m_statsFrames << " paints/s, paint avg "
           << (m_statsPaintTotalNs / m_statsFrames) / 1e6 << " ms, max " << m_statsPaintMaxNs / 1e6
           << " ms, longest frame interval " << m_statsFrameMaxNs / 1e6 << " ms, view " << width() << "x" << height();

    m_statsSinceNs = now;
    m_statsFrames = 0;
    m_statsPaintTotalNs = 0;
    m_statsPaintMaxNs = 0;
    m_statsFrameMaxNs = 0;
}
