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
#include <QPointF>
#include <QVariantList>
#include <qqmlintegration.h>

#include "modularity/ioc.h"
#include "async/asyncable.h"

#include "context/iglobalcontext.h"
#include "playback/iplaybackcontroller.h"
#include "engraving/types/types.h"

namespace mu::appshell {
//! Audible scrubbing through the score (chord scrubbing tool): while the finger is held on the score
//! (see NotationView.reportScrubs) everything that sounds at the position of the finger (all tracks) is played as a
//! chord, and the chord follows the finger. The playhead does not move.
//! `noteRects` are the areas of all sounding notes that are shown (logical score coordinates of the shown score) for
//! the views to point at them; it is empty while nothing sounds.
//! The session mix (mute, solo, faders) applies, because the notes go through the tracks of the playback.
class ChordScrubber : public QObject, public muse::Contextable, public muse::async::Asyncable
{
    Q_OBJECT

    Q_PROPERTY(QVariantList noteRects READ noteRects NOTIFY noteRectsChanged)
    //! Only the notes that are in the shown part sound (instead of all instruments of the score)
    Q_PROPERTY(bool shownPartOnly READ shownPartOnly WRITE setShownPartOnly NOTIFY shownPartOnlyChanged)

    QML_ELEMENT

    muse::ContextInject<context::IGlobalContext> globalContext = { this };
    muse::ContextInject<playback::IPlaybackController> playbackController = { this };

public:
    explicit ChordScrubber(QObject* parent = nullptr);
    ~ChordScrubber() override;

    QVariantList noteRects() const;

    bool shownPartOnly() const;
    void setShownPartOnly(bool only);

    //! Positions in logical coordinates of the score view
    Q_INVOKABLE void begin(const QPointF& logicPos);
    Q_INVOKABLE void move(const QPointF& logicPos);
    Q_INVOKABLE void end();

signals:
    void noteRectsChanged();
    void shownPartOnlyChanged();

private:
    void setNoteRects(const QVariantList& rects);
    void playAt(const QPointF& logicPos);
    void stopSound();

    bool m_active = false;
    int m_lastTick = -1;
    bool m_shownPartOnly = false;
    QVariantList m_noteRects;
    engraving::InstrumentTrackIdSet m_soundingTracks;
};
}
