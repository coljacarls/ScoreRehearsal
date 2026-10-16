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
#include <qqmlintegration.h>

#include "modularity/ioc.h"
#include "async/asyncable.h"

#include "context/iglobalcontext.h"
#include "playback/iplaybackcontroller.h"
#include "engraving/types/types.h"

namespace mu::appshell {
//! Audible scrubbing through the score: while the finger is held on the score (see NotationView.reportScrubs),
//! the playhead follows it and everything that sounds at the playhead position (all tracks) is played as a chord.
//! The session mix (mute, solo, faders) applies, because the notes go through the tracks of the playback.
class ChordScrubber : public QObject, public muse::Contextable, public muse::async::Asyncable
{
    Q_OBJECT

    QML_ELEMENT

    muse::ContextInject<context::IGlobalContext> globalContext = { this };
    muse::ContextInject<playback::IPlaybackController> playbackController = { this };

public:
    explicit ChordScrubber(QObject* parent = nullptr);
    ~ChordScrubber() override;

    //! Positions in logical coordinates of the score view
    Q_INVOKABLE void begin(const QPointF& logicPos);
    Q_INVOKABLE void move(const QPointF& logicPos);
    Q_INVOKABLE void end();

private:
    void playAt(const QPointF& logicPos);
    void stopSound();

    bool m_active = false;
    int m_lastTick = -1;
    engraving::InstrumentTrackIdSet m_soundingTracks;
};
}
