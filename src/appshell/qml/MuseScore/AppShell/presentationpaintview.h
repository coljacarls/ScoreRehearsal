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
#include <vector>

#include <QColor>
#include <QElapsedTimer>
#include <QImage>
#include <QQuickWindow>
#include <QRectF>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <qqmlintegration.h>

#include "async/asyncable.h"
#include "context/iglobalcontext.h"
#include "draw/types/geometry.h"
#include "draw/types/transform.h"
#include "modularity/ioc.h"
#include "notation/inotationconfiguration.h"
#include "notationscene/qml/MuseScore/NotationScene/abstractnotationpaintview.h"
#include "ui/iuiconfiguration.h"


namespace mu::notation {
class PlaybackCursor;
class ContinuousPanel;
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
//!
//! The score is not painted per frame: after each resize (and whenever the content changes) it is
//! rendered once into tiles (images, uploaded as textures) - first the ones in view, then the rest
//! while the application is idle. A frame only moves the tiles underneath the playhead.
//! The sticky panel (instrument names, clef, key) is rendered the same way, when the measure under
//! it changes.
class PresentationPaintView : public notation::AbstractNotationPaintView
{
    Q_OBJECT

    Q_PROPERTY(QRectF playheadRect READ playheadRect NOTIFY playheadRectChanged)
    Q_PROPERTY(QColor playheadColor READ playheadColor CONSTANT)
    Q_PROPERTY(QStringList soloPartIds READ soloPartIds WRITE setSoloPartIds NOTIFY soloPartIdsChanged)
    Q_PROPERTY(QVariantList shadeBands READ shadeBands NOTIFY shadeBandsChanged)

    QML_ELEMENT

    muse::GlobalInject<notation::INotationConfiguration> notationConfig;
    muse::GlobalInject<muse::ui::IUiConfiguration> uiConfig;
    muse::ContextInject<context::IGlobalContext> globalContext = { this };

public:
    explicit PresentationPaintView(QQuickItem* parent = nullptr);
    ~PresentationPaintView() override;

    //! In view coordinates; empty while the position is unknown
    QRectF playheadRect() const;
    QColor playheadColor() const;

    //! Parts to keep lit (the solo parts of the rehearsal view); everything else is shaded. Empty: no shade.
    QStringList soloPartIds() const;
    void setSoloPartIds(const QStringList& ids);

    //! The shaded stripes of the window height, in view coordinates, top to bottom:
    //! [{ y, height, fadeTop, fadeBottom }]. The fade is the length of the gradient to the lit staves.
    QVariantList shadeBands() const;

signals:
    void playheadRectChanged();
    void soloPartIdsChanged();
    void shadeBandsChanged();

private slots:
    void onWindowChanged(QQuickWindow* window);
    void onFrame();

private:
    bool followPlaybackCursor(const muse::RectF& cursorRect) override;
    void onViewSizeChanged() override;
    void onLoadNotation(notation::INotationPtr notation) override;
    void onUnloadNotation(notation::INotationPtr notation) override;
    void onMatrixChanged(const muse::draw::Transform& oldMatrix, const muse::draw::Transform& newMatrix,
                         bool overrideZoomType) override;
    QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData* data) override;

    //! The view does not use the zoom and position stored in the (shared) view state
    void initZoomAndPosition() override;

    //! Scales the score to the window height and puts the cursor at its fixed place
    void alignToCursor(const muse::RectF& cursorRect);
    void updatePlayhead(const muse::RectF& cursorRect);

    //! The vertical position of the staves does not change while playing: the stripes only
    //! need to be calculated again when the scaling, the score or the solo parts change
    void updateShadeBands();

    //! Width of about one measure (in logical units)
    qreal measureWidth() const;

    void onPlaybackPositionChanged(muse::secs_t secs);
    void requestFrame();
    void logStats();

    //! One horizontal slice of the score, rendered with the scaling of the generation it belongs to
    struct Tile {
        QImage pending;         // rendered, not yet uploaded to the scene graph
        bool resident = false;  // rendered (or about to be) and kept
    };

    //! Where and how the tiles are rendered (everything in logical units / device pixels)
    struct TileGeometry {
        muse::PointF origin;    // top left of the tiles in logical coordinates
        qreal scalePx = 0.0;    // device pixels per logical unit
        qreal dpr = 1.0;
        int tileWidthPx = 0;
        int heightPx = 0;
        int count = 0;
        int serial = 0;         // changes with every new set of tiles
    };

    void scheduleRebuild(int delayMs);
    void rebuildTiles();
    void renderTile(int index);
    void renderNextTile();
    void scheduleRenderStep();
    std::vector<int> tilesInViewport() const;
    int tileAtLogicalX(qreal x) const;
    int maxResidentTiles() const;

    void rebuildMeasureStarts();
    void updatePanelIfNeeded();
    void renderPanel(qreal offsetPanel);

    std::unique_ptr<notation::PlaybackCursor> m_cursor;
    muse::async::Asyncable m_subscriptions;

    muse::RectF m_lastCursorRect;

    QStringList m_soloPartIds;
    QVariantList m_shadeBands;
    qreal m_shadeScaling = 0.0;
    bool m_shadeDirty = true;
    QRectF m_playheadRect;

    //! Vertical extent of the system (in logical units), known after the first cursor position
    qreal m_systemCenterY = 0.0;
    bool m_hasSystemCenter = false;

    std::vector<Tile> m_tiles;
    TileGeometry m_geometry;
    QTimer m_rebuildTimer;
    QTimer m_renderTimer;

    //! The sticky panel, rendered into an image of its own
    std::unique_ptr<notation::ContinuousPanel> m_panel;
    QImage m_panelPending;
    bool m_panelValid = false;
    bool m_panelInTransition = false;
    bool m_panelFirstMeasureInView = false;
    int m_panelMeasure = -1;
    qreal m_panelWidth = 0.0;
    std::vector<qreal> m_measureStarts;

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
    qint64 m_statsFrameMaxNs = 0;
    qint64 m_statsTileNs = 0;
    int m_statsTiles = 0;
    int m_statsFrames = 0;
};
}
