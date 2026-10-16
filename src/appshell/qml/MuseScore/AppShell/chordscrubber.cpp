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

#include "chordscrubber.h"

#include <map>
#include <vector>

#include "engraving/dom/chord.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/note.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/tie.h"
#include "engraving/dom/utils.h"

#include "notation/imasternotation.h"
#include "notation/inotation.h"
#include "notation/inotationelements.h"
#include "notation/inotationplayback.h"

using namespace mu::appshell;
using namespace mu::engraving;

namespace {
//! Long enough to sound until the finger is lifted (the sound is cut then)
constexpr muse::mpe::duration_t HOLD_DURATION = 600ll * 1000 * 1000;

//! The first note of a tie chain: only that one starts a sound
const Note* tieStart(const Note* note)
{
    while (note->tieBack() && note->tieBack()->startNote()) {
        note = note->tieBack()->startNote();
    }

    return note;
}

//! Flushing the off stream with no events cuts the sound of the track
void silence(const mu::notation::INotationPlaybackPtr& playback, const InstrumentTrackId& trackId)
{
    muse::mpe::OffStreamChanges offStream = playback->trackPlaybackData(trackId).offStream; // shares the channel
    offStream.send(muse::mpe::PlaybackEventsMap(), true);
}
}

ChordScrubber::ChordScrubber(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

ChordScrubber::~ChordScrubber()
{
    if (m_active) {
        stopSound();
    }
}

void ChordScrubber::begin(const QPointF& logicPos)
{
    if (playbackController()->isPlaying()) {
        playbackController()->pause();
    }

    m_active = true;
    m_lastTick = -1;
    playAt(logicPos);
}

void ChordScrubber::move(const QPointF& logicPos)
{
    if (m_active) {
        playAt(logicPos);
    }
}

void ChordScrubber::end()
{
    if (!m_active) {
        return;
    }

    m_active = false;
    stopSound();
}

void ChordScrubber::playAt(const QPointF& logicPos)
{
    const notation::INotationPtr notation = globalContext()->currentNotation();
    const notation::IMasterNotationPtr master = globalContext()->currentMasterNotation();
    if (!notation || !notation->elements() || !master || !master->masterScore() || !master->playback()) {
        return;
    }

    //! The shown score can be a part: its position gives the tick, the sound comes from the full score
    const Score* shownScore = notation->elements()->msScore();
    Segment* shownSegment = nullptr;
    const Measure* measure = shownScore->pos2measure(muse::PointF::fromQPointF(logicPos), nullptr, nullptr, &shownSegment, nullptr);
    if (!measure) {
        return;
    }

    const Fraction tick = shownSegment ? shownSegment->tick() : measure->tick();
    if (tick.ticks() == m_lastTick) {
        return;
    }

    const MasterScore* score = master->masterScore();
    const Segment* segment = score->tick2segment(tick, true, SegmentType::ChordRest);
    if (!segment) {
        return;
    }

    m_lastTick = tick.ticks();

    //! Everything that sounds at the tick, also long notes that started before
    std::map<InstrumentTrackId, std::vector<const EngravingItem*> > notesByTrack;
    for (track_idx_t track = 0; track < score->ntracks(); ++track) {
        const ChordRest* cr = score->findCR(tick, track);
        if (!cr || !cr->isChord()) {
            continue;
        }

        for (const Note* note : toChord(cr)->notes()) {
            const Note* start = tieStart(note);
            if (!start->play()) {
                continue;
            }

            notesByTrack[makeInstrumentTrackId(start)].push_back(start);
        }
    }

    const EngravingItem* anchor = nullptr;
    for (track_idx_t track = 0; track < score->ntracks() && !anchor; ++track) {
        anchor = segment->element(track);
    }

    if (anchor) {
        //! No flush: a seek that flushes the sound would also cut the chord that is triggered right after it
        //! (the order of both on the audio thread is not guaranteed). The tracks are flushed by their own triggers.
        playbackController()->seekElement(anchor, false);
    }

    const notation::INotationPlaybackPtr playback = master->playback();
    InstrumentTrackIdSet nowSounding;
    for (const auto& [trackId, items] : notesByTrack) {
        playback->triggerEventsForItems(items, HOLD_DURATION, true);
        nowSounding.insert(trackId);
    }

    //! Tracks that sounded before and are silent now
    for (const InstrumentTrackId& trackId : m_soundingTracks) {
        if (!nowSounding.count(trackId)) {
            silence(playback, trackId);
        }
    }

    m_soundingTracks = nowSounding;
}

void ChordScrubber::stopSound()
{
    const notation::IMasterNotationPtr master = globalContext()->currentMasterNotation();
    if (master && master->playback()) {
        for (const InstrumentTrackId& trackId : m_soundingTracks) {
            silence(master->playback(), trackId);
        }
    }

    m_soundingTracks.clear();
}
