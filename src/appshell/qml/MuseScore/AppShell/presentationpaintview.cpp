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

#include <QMatrix4x4>
#include <QPainter>
#include <QQuickWindow>
#include <QSGRectangleNode>
#include <QSGSimpleTextureNode>
#include <QSGTransformNode>
#include <QtGlobal>

#include "draw/painter.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/page.h"
#include "engraving/dom/score.h"
#include "engraving/dom/system.h"
#include "notation/inotationelements.h"
#include "notation/inotationpainting.h"
#include "notation/inotationplayback.h"
#include "notation/types/viewmode.h"
#include "notationscene/qml/MuseScore/NotationScene/continuouspanel.h"
#include "notationscene/qml/MuseScore/NotationScene/playbackcursor.h"

#include "presentationsettings.h"

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

//! Width of a tile of the rendered score, in device pixels
constexpr int TILE_WIDTH_PX = 2048;

//! The tiles are rendered again this long after the last change (e.g. of the window size)
constexpr int REBUILD_DELAY_MS = 150;

//! Pause between two tiles that are rendered in the background (the GUI thread is busy while rendering)
constexpr int RENDER_STEP_MS = 10;
constexpr int RENDER_STEP_PLAYING_MS = 40;
constexpr int RENDER_WATCH_MS = 250;

//! Memory the tiles of the score may take up (as images, before the upload to the GPU)
constexpr qint64 TILE_MEMORY_BUDGET = 256ll * 1024 * 1024;

//! Width of the image of the sticky panel, relative to the width of the view
constexpr qreal PANEL_IMAGE_WIDTH_RATIO = 0.6;

//! The scene graph of the view: background, the tiles of the score and the sticky panel
struct SceneRoot : public QSGNode {
    QSGRectangleNode* background = nullptr;
    QSGTransformNode* tileLayer = nullptr;
    QSGTransformNode* panelLayer = nullptr;
    QSGSimpleTextureNode* panel = nullptr;
    std::vector<QSGSimpleTextureNode*> tiles;
    int serial = -1;
};
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

    m_rebuildTimer.setSingleShot(true);
    connect(&m_rebuildTimer, &QTimer::timeout, this, &PresentationPaintView::rebuildTiles);

    m_renderTimer.setSingleShot(true);
    connect(&m_renderTimer, &QTimer::timeout, this, &PresentationPaintView::renderNextTile);

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
    m_staffCountDirty = true;

    //! The sticky panel is rendered by this view as well (not painted by the base class)
    if (!m_panel) {
        m_panel = std::make_unique<ContinuousPanel>();
    }

    m_panel->setNotation(notation);

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

    //! The rendered score is outdated when the content or the colors change
    notation->notationChanged().onReceive(&m_subscriptions, [this](const RectF&) {
        m_staffCountDirty = true;
        scheduleRebuild(REBUILD_DELAY_MS);
    }, async::Asyncable::Mode::SetReplace);

    const auto rebuild = [this]() {
        scheduleRebuild(REBUILD_DELAY_MS);
    };

    //! The scaling changes with the maximum staff size
    settings()->valueChanged(maxStaffHeightKey()).onReceive(&m_subscriptions, [this](const Val&) {
        if (m_lastCursorRect.isValid()) {
            alignToCursor(m_lastCursorRect);
        }

        scheduleRebuild(REBUILD_DELAY_MS);
    }, async::Asyncable::Mode::SetReplace);

    notationConfig()->foregroundChanged().onNotify(&m_subscriptions, rebuild, async::Asyncable::Mode::SetReplace);
    notationConfig()->backgroundChanged().onNotify(&m_subscriptions, rebuild, async::Asyncable::Mode::SetReplace);
    notationConfig()->notationColorChanged().onNotify(&m_subscriptions, rebuild, async::Asyncable::Mode::SetReplace);
    notationConfig()->scoreInversionChanged().onNotify(&m_subscriptions, rebuild, async::Asyncable::Mode::SetReplace);
    notationConfig()->isOnlyInvertInDarkThemeChanged().onNotify(&m_subscriptions, rebuild, async::Asyncable::Mode::SetReplace);
    uiConfig()->currentThemeChanged().onNotify(&m_subscriptions, rebuild, async::Asyncable::Mode::SetReplace);
}

void PresentationPaintView::onUnloadNotation(INotationPtr notation)
{
    globalContext()->playbackState()->playbackPositionChanged().disconnect(&m_subscriptions);
    globalContext()->playbackState()->playbackStatusChanged().disconnect(&m_subscriptions);
    notation->notationChanged().disconnect(&m_subscriptions);
    settings()->valueChanged(maxStaffHeightKey()).disconnect(&m_subscriptions);
    notationConfig()->foregroundChanged().disconnect(&m_subscriptions);
    notationConfig()->backgroundChanged().disconnect(&m_subscriptions);
    notationConfig()->notationColorChanged().disconnect(&m_subscriptions);
    notationConfig()->scoreInversionChanged().disconnect(&m_subscriptions);
    notationConfig()->isOnlyInvertInDarkThemeChanged().disconnect(&m_subscriptions);
    uiConfig()->currentThemeChanged().disconnect(&m_subscriptions);

    if (m_cursor) {
        m_cursor->setNotation(nullptr);
    }

    if (m_panel) {
        m_panel->setNotation(nullptr);
    }

    //! Dropping the tiles also removes their nodes from the scene graph (the serial changes)
    m_rebuildTimer.stop();
    m_renderTimer.stop();
    m_tiles.clear();
    m_geometry = TileGeometry();
    m_geometry.serial = m_geometry.serial + 1;
    m_measureStarts.clear();
    m_panelPending = QImage();
    m_panelValid = false;
    m_panelInTransition = false;
    m_panelMeasure = -1;
    m_panelWidth = 0.0;
    m_hasSystemCenter = false;

    m_lastCursorRect = RectF();
    m_clockRunning = false;

    if (!m_playheadRect.isNull()) {
        m_playheadRect = QRectF();
        emit playheadRectChanged();
    }

    AbstractNotationPaintView::onUnloadNotation(notation);
    update();
}

void PresentationPaintView::onWindowChanged(QQuickWindow* window)
{
    if (!window) {
        return;
    }

    //! Emitted after the animations of a frame were advanced, right before the frame is rendered
    connect(window, &QQuickWindow::afterAnimating, this, &PresentationPaintView::onFrame, Qt::UniqueConnection);

    //! A window on another screen can have another pixel ratio: the tiles have to match it
    connect(window, &QWindow::screenChanged, this, [this]() {
        scheduleRebuild(REBUILD_DELAY_MS);
    }, Qt::UniqueConnection);
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

    if (m_statsEnabled) {
        if (sinceLastFrameNs < 1000000000) {
            m_statsFrameMaxNs = std::max(m_statsFrameMaxNs, sinceLastFrameNs);
        }

        logStats();
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

    //! The tiles that exist are shown stretched until the new ones are ready
    scheduleRebuild(REBUILD_DELAY_MS);
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
        qreal scaling = height() / systemHeight;

        //! A staff is not enlarged beyond the set share of the window height: a score with a single
        //! staff gets white space above and below instead of notes that are too large
        const int staves = visibleStaffCount();
        if (staves > 0) {
            const qreal staffHeight = cursorRect.height() / staves;
            scaling = std::min(scaling, maxStaffHeightPercent() / 100.0 * height() / staffHeight);
        }

        setScaling(scaling, PointF());
    }

    //! The tiles are rendered for this vertical position: as soon as it is known, they can be made
    if (!m_hasSystemCenter || !qFuzzyCompare(m_systemCenterY, cursorRect.center().y())) {
        m_systemCenterY = cursorRect.center().y();
        m_hasSystemCenter = true;
        scheduleRebuild(m_tiles.empty() ? 0 : REBUILD_DELAY_MS);
    }

    const RectF view = viewport();

    //! The playhead sits behind the sticky panel (instrument names, clef) and about one measure
    //! of notes that were already sung
    const qreal gap = std::clamp(measureWidth(), view.width() * MIN_GAP_RATIO, view.width() * MAX_GAP_RATIO);
    const qreal panelWidth = m_panelValid ? m_panelWidth : continuousPanelWidth();
    const qreal left = cursorRect.left() - panelWidth - gap;
    const qreal top = cursorRect.center().y() - view.height() / 2;

    moveCanvasToPosition(PointF(left, top));
    updatePlayhead(cursorRect);
}

int PresentationPaintView::visibleStaffCount()
{
    //! The layout can be redone (another part, another system) without a change notification
    const engraving::Score* current = notation() ? notation()->elements()->msScore() : nullptr;
    const engraving::System* currentSystem = current && !current->systems().empty() ? current->systems().front() : nullptr;
    if (currentSystem != m_countedSystem) {
        m_countedSystem = currentSystem;
        m_staffCountDirty = true;
    }

    if (m_staffCountDirty) {
        m_staffCountDirty = false;
        m_visibleStaffCount = 0;

        const engraving::Score* score = notation() ? notation()->elements()->msScore() : nullptr;
        if (score && !score->systems().empty()) {
            for (const engraving::SysStaff* staff : score->systems().front()->staves()) {
                if (staff->show()) {
                    ++m_visibleStaffCount;
                }
            }
        }
    }

    return m_visibleStaffCount;
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

void PresentationPaintView::onMatrixChanged(const muse::draw::Transform& oldMatrix, const muse::draw::Transform& newMatrix,
                                            bool overrideZoomType)
{
    AbstractNotationPaintView::onMatrixChanged(oldMatrix, newMatrix, overrideZoomType);
    updatePanelIfNeeded();
}

void PresentationPaintView::scheduleRebuild(int delayMs)
{
    //! A running timer is restarted: while the window is being resized, nothing is rendered
    m_rebuildTimer.start(delayMs);
}

void PresentationPaintView::rebuildTiles()
{
    if (!notation() || !m_hasSystemCenter || !window() || width() <= 0 || height() <= 0) {
        return;
    }

    const engraving::Score* score = notation()->elements()->msScore();
    if (!score || score->pages().empty()) {
        return;
    }

    const engraving::Page* page = score->pages().front();
    const RectF scoreRect = page->ldata()->bbox().translated(page->pos());
    const qreal scaling = currentScaling();
    if (scoreRect.width() <= 0.0 || scaling <= 0.0) {
        return;
    }

    TileGeometry g;
    g.dpr = window()->effectiveDevicePixelRatio();
    g.scalePx = scaling * g.dpr;
    g.tileWidthPx = TILE_WIDTH_PX;
    g.heightPx = static_cast<int>(std::ceil(height() * g.dpr));

    //! The same vertical section of the score that alignToCursor() puts into the window
    g.origin = PointF(scoreRect.left(), m_systemCenterY - (height() / 2.0) / scaling);
    g.count = static_cast<int>(std::ceil(scoreRect.width() * g.scalePx / g.tileWidthPx));
    g.serial = m_geometry.serial + 1;

    m_geometry = g;
    m_tiles.assign(g.count, Tile());
    rebuildMeasureStarts();

    //! What is in view has to be there with the next frame
    for (int i : tilesInViewport()) {
        renderTile(i);
    }

    m_panelMeasure = -1;
    updatePanelIfNeeded();

    //! The width of the panel is known now: puts the playhead at its place
    if (m_lastCursorRect.isValid()) {
        alignToCursor(m_lastCursorRect);
    }

    update();
    scheduleRenderStep();
}

std::vector<int> PresentationPaintView::tilesInViewport() const
{
    std::vector<int> result;
    if (m_tiles.empty()) {
        return result;
    }

    const RectF view = viewport();
    const int last = static_cast<int>(m_tiles.size()) - 1;
    const int from = std::clamp(tileAtLogicalX(view.left()), 0, last);
    const int to = std::clamp(tileAtLogicalX(view.right()), 0, last);
    for (int i = from; i <= to; ++i) {
        result.push_back(i);
    }

    return result;
}

int PresentationPaintView::tileAtLogicalX(qreal x) const
{
    const qreal tileWidth = m_geometry.tileWidthPx / m_geometry.scalePx;
    return static_cast<int>(std::floor((x - m_geometry.origin.x()) / tileWidth));
}

int PresentationPaintView::maxResidentTiles() const
{
    const qint64 tileBytes = static_cast<qint64>(m_geometry.tileWidthPx) * m_geometry.heightPx * 4;
    return static_cast<int>(std::max<qint64>(TILE_MEMORY_BUDGET / std::max<qint64>(tileBytes, 1), 3));
}

void PresentationPaintView::renderTile(int index)
{
    if (index < 0 || index >= static_cast<int>(m_tiles.size()) || !notation()) {
        return;
    }

    QElapsedTimer timer;
    timer.start();

    const TileGeometry& g = m_geometry;

    QImage image(g.tileWidthPx, g.heightPx, QImage::Format_RGB32);
    image.fill(notationConfig()->backgroundColor());

    {
        QPainter qp(&image);
        muse::draw::Painter painter(&qp, "presentation_tile");

        //! logical -> pixels of this tile
        muse::draw::Transform transform;
        transform.translate(-g.origin.x() * g.scalePx - index * static_cast<qreal>(g.tileWidthPx), -g.origin.y() * g.scalePx);
        transform.scale(g.scalePx, g.scalePx);
        painter.setWorldTransform(transform);

        const qreal tileWidth = g.tileWidthPx / g.scalePx;
        const RectF frame(g.origin.x() + index * tileWidth, g.origin.y(), tileWidth, g.heightPx / g.scalePx);

        //! Printing mode like in the other views of this app (read-only): no selection, no UI overlays
        notation()->painting()->paintView(&painter, frame.padded(1), true, false);
    }

    Tile& tile = m_tiles[index];
    tile.pending = image;
    tile.resident = true;

    m_statsTileNs += timer.nsecsElapsed();
    ++m_statsTiles;
}

void PresentationPaintView::scheduleRenderStep()
{
    if (m_tiles.empty() || m_renderTimer.isActive()) {
        return;
    }

    const bool playing = globalContext()->playbackState()->isPlaying();
    m_renderTimer.start(playing ? RENDER_STEP_PLAYING_MS : RENDER_STEP_MS);
}

void PresentationPaintView::renderNextTile()
{
    if (m_tiles.empty() || !notation()) {
        return;
    }

    const int count = static_cast<int>(m_tiles.size());
    const int center = std::clamp(tileAtLogicalX(viewport().left()), 0, count - 1);

    //! The whole score if it fits into the memory budget, otherwise a window around the view
    //! (mostly ahead of it: the score moves from right to left)
    const int budget = maxResidentTiles();
    int from = 0;
    int to = count - 1;
    if (count > budget) {
        from = std::max(center - budget / 4, 0);
        to = std::min(from + budget - 1, count - 1);
        from = std::max(to - budget + 1, 0);
    }

    for (int i = 0; i < count; ++i) {
        Tile& tile = m_tiles[i];
        if (tile.resident && (i < from || i > to)) {
            tile.resident = false;
            tile.pending = QImage();
        }
    }

    //! Ahead of the playhead first, then what has already been played
    int next = -1;
    for (int i = std::max(center, from); i <= to && next < 0; ++i) {
        if (!m_tiles[i].resident) {
            next = i;
        }
    }

    for (int i = std::min(center, to) - 1; i >= from && next < 0; --i) {
        if (!m_tiles[i].resident) {
            next = i;
        }
    }

    if (next >= 0) {
        renderTile(next);
        update();
        scheduleRenderStep();
        return;
    }

    //! Everything is there. A score that does not fit into the budget keeps being watched,
    //! the window of tiles moves with the playback position
    if (count > budget) {
        m_renderTimer.start(RENDER_WATCH_MS);
    }
}

void PresentationPaintView::rebuildMeasureStarts()
{
    m_measureStarts.clear();

    const engraving::Score* score = notation() ? notation()->elements()->msScore() : nullptr;
    if (!score) {
        return;
    }

    for (const engraving::Measure* m = score->firstMeasureMM(); m; m = m->nextMeasureMM()) {
        m_measureStarts.push_back(m->canvasX());
    }
}

void PresentationPaintView::updatePanelIfNeeded()
{
    if (!m_panel || m_geometry.count == 0 || !window()) {
        return;
    }

    //! The sticky panel shows the clef, key etc. of the measure under it, and slides over to those
    //! of the next measure at the boundary: it only has to be rendered again when that changes
    const qreal offsetPanel = viewport().left();
    const qreal probe = offsetPanel + m_panelWidth;
    const auto it = std::upper_bound(m_measureStarts.begin(), m_measureStarts.end(), probe);
    const int measure = static_cast<int>(it - m_measureStarts.begin()) - 1;

    //! In front of the first measure there is no panel (the instrument names are in the score)
    const bool firstMeasureInView = !m_measureStarts.empty() && offsetPanel < m_measureStarts.front();

    if (measure != m_panelMeasure || firstMeasureInView != m_panelFirstMeasureInView || m_panelInTransition) {
        m_panelMeasure = measure;
        m_panelFirstMeasureInView = firstMeasureInView;
        renderPanel(offsetPanel);
    }
}

void PresentationPaintView::renderPanel(qreal offsetPanel)
{
    const TileGeometry& g = m_geometry;
    const qreal scaling = currentScaling();
    if (g.count == 0 || scaling <= 0.0) {
        return;
    }

    const int widthPx = static_cast<int>(std::ceil(width() * PANEL_IMAGE_WIDTH_RATIO * g.dpr));
    QImage image(widthPx, g.heightPx, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    {
        QPainter qp(&image);
        muse::draw::Painter painter(&qp, "presentation_panel");

        //! logical -> pixels, with the left edge of the view at 0
        muse::draw::Transform transform;
        transform.translate(-offsetPanel * g.scalePx, -g.origin.y() * g.scalePx);
        transform.scale(g.scalePx, g.scalePx);
        painter.setWorldTransform(transform);

        const PointF origin = g.origin;

        ContinuousPanel::NotationViewContext ctx;
        ctx.xOffset = -offsetPanel * scaling;
        ctx.yOffset = -origin.y() * scaling;
        ctx.scaling = scaling;
        ctx.fromLogical = [offsetPanel, scaling, origin](const PointF& pos) {
            return PointF((pos.x() - offsetPanel) * scaling, (pos.y() - origin.y()) * scaling);
        };

        engraving::rendering::PaintOptions opt;
        opt.invertColors = notationConfig()->shouldInvertScore();
        m_panel->paint(painter, ctx, opt);
    }

    //! While the width is still changing, the panel is sliding and has to follow the position
    m_panelInTransition = !qFuzzyCompare(m_panel->width() + 1.0, m_panelWidth + 1.0);
    m_panelWidth = m_panel->width();
    m_panelPending = image;
    m_panelValid = true;
    update();
}

QSGNode* PresentationPaintView::updatePaintNode(QSGNode* old, UpdatePaintNodeData*)
{
    //! NOTE: runs on the render thread while the GUI thread waits, the members can be read here.
    //! The base class would paint the whole score into a texture at this place, this view only
    //! moves textures that were rendered before.
    QQuickWindow* win = window();
    if (!win) {
        return old;
    }

    SceneRoot* root = static_cast<SceneRoot*>(old);
    if (!root) {
        root = new SceneRoot();
        root->background = win->createRectangleNode();
        root->tileLayer = new QSGTransformNode();
        root->panelLayer = new QSGTransformNode();

        root->appendChildNode(root->background);
        root->appendChildNode(root->tileLayer);
        root->appendChildNode(root->panelLayer);
    }

    root->background->setRect(QRectF(0, 0, width(), height()));
    root->background->setColor(notationConfig()->backgroundColor());

    //! A new set of tiles (another size or content): the old ones go
    const TileGeometry& g = m_geometry;
    if (root->serial != g.serial) {
        qDeleteAll(root->tiles);
        root->tiles.assign(m_tiles.size(), nullptr);
        root->serial = g.serial;
    }

    for (size_t i = 0; i < m_tiles.size() && i < root->tiles.size(); ++i) {
        Tile& tile = m_tiles[i];
        QSGSimpleTextureNode*& node = root->tiles[i];

        if (!tile.pending.isNull()) {
            if (!node) {
                node = new QSGSimpleTextureNode();
                node->setFiltering(QSGTexture::Linear);
                node->setOwnsTexture(true);
                node->setRect(QRectF(i * static_cast<qreal>(g.tileWidthPx), 0, g.tileWidthPx, g.heightPx));
                root->tileLayer->appendChildNode(node);
            }

            node->setTexture(win->createTextureFromImage(tile.pending, QQuickWindow::TextureIsOpaque));
            tile.pending = QImage();
        } else if (!tile.resident && node) {
            delete node;
            node = nullptr;
        }
    }

    if (!m_panelPending.isNull()) {
        if (!root->panel) {
            root->panel = new QSGSimpleTextureNode();
            root->panel->setFiltering(QSGTexture::Linear);
            root->panel->setOwnsTexture(true);
            root->panelLayer->appendChildNode(root->panel);
        }

        root->panel->setRect(QRectF(0, 0, m_panelPending.width(), m_panelPending.height()));
        root->panel->setTexture(win->createTextureFromImage(m_panelPending));
        m_panelPending = QImage();
    }

    //! Everything is placed with the current matrix of the view: pixels of the tiles -> view
    if (g.scalePx > 0.0) {
        const PointF origin = fromLogical(g.origin);
        const float k = static_cast<float>(currentScaling() / g.scalePx);

        QMatrix4x4 tileMatrix;
        tileMatrix.translate(static_cast<float>(origin.x()), static_cast<float>(origin.y()));
        tileMatrix.scale(k, k);
        root->tileLayer->setMatrix(tileMatrix);

        //! The panel always sits at the left edge of the view
        QMatrix4x4 panelMatrix;
        panelMatrix.translate(0.0f, static_cast<float>(origin.y()));
        panelMatrix.scale(k, k);
        root->panelLayer->setMatrix(panelMatrix);
    }

    return root;
}

void PresentationPaintView::logStats()
{
    const qint64 now = m_clock.nsecsElapsed();
    ++m_statsFrames;

    if (now - m_statsSinceNs < 1000000000) {
        return;
    }

    LOGI() << "presentation view: " << m_statsFrames << " frames/s, " << m_statsTiles << " tiles rendered ("
           << m_statsTileNs / 1e6 << " ms), longest frame interval " << m_statsFrameMaxNs / 1e6
           << " ms, view " << width() << "x" << height() << ", tiles " << m_tiles.size();

    m_statsSinceNs = now;
    m_statsFrames = 0;
    m_statsTiles = 0;
    m_statsTileNs = 0;
    m_statsFrameMaxNs = 0;
}
