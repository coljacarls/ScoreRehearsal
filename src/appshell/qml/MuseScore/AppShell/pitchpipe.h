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
#include <QRectF>
#include <qqmlintegration.h>

#include "modularity/ioc.h"
#include "async/asyncable.h"

#include "context/iglobalcontext.h"
#include "playback/iplaybackcontroller.h"
#include "engraving/types/types.h"

namespace mu::engraving {
class Note;
}

namespace mu::appshell {
//! The pitch pipe tool: a note that is touched sounds as long as the finger is held on it, and while the finger
//! moves over the score every note it passes sounds in turn (see NotationView.reportScrubs with scrubDelay 0).
//! `noteRect` is the area of the sounding note (logical score coordinates of the shown score) for the
//! views to point at it; it is empty while no note sounds.
//! The session mix (mute, solo, faders) applies, because the note goes through the track of the playback.
class PitchPipe : public QObject, public muse::Contextable, public muse::async::Asyncable
{
    Q_OBJECT

    Q_PROPERTY(bool sounding READ sounding NOTIFY noteRectChanged)
    Q_PROPERTY(QRectF noteRect READ noteRect NOTIFY noteRectChanged)

    QML_ELEMENT

    muse::ContextInject<context::IGlobalContext> globalContext = { this };
    muse::ContextInject<playback::IPlaybackController> playbackController = { this };

public:
    explicit PitchPipe(QObject* parent = nullptr);
    ~PitchPipe() override;

    bool sounding() const;
    QRectF noteRect() const;

    //! Positions in logical coordinates of the score view
    Q_INVOKABLE void begin(const QPointF& logicPos);
    Q_INVOKABLE void move(const QPointF& logicPos);
    Q_INVOKABLE void end();

signals:
    void noteRectChanged();

private:
    void playAt(const QPointF& logicPos);
    const engraving::Note* noteAt(const QPointF& logicPos) const;
    void stopSound();

    bool m_active = false;
    const engraving::Note* m_note = nullptr;
    QRectF m_noteRect;
    engraving::InstrumentTrackId m_soundingTrack;
    bool m_hasSoundingTrack = false;
};
}
