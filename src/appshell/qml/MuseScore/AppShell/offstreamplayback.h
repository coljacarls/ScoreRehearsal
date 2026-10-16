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

#include "engraving/dom/note.h"
#include "engraving/dom/tie.h"
#include "engraving/types/types.h"
#include "notation/inotationplayback.h"

//! Helpers to play notes of the score without the playback (off stream) and to cut them again
namespace mu::appshell {
//! Long enough to sound until the finger is lifted (the sound is cut then)
constexpr muse::mpe::duration_t HOLD_DURATION = 600ll * 1000 * 1000;

//! The first note of a tie chain: only that one starts a sound
inline const engraving::Note* tieStart(const engraving::Note* note)
{
    while (note->tieBack() && note->tieBack()->startNote()) {
        note = note->tieBack()->startNote();
    }

    return note;
}

//! Flushing the off stream with no events cuts the sound of the track
inline void silenceTrack(const notation::INotationPlaybackPtr& playback, const engraving::InstrumentTrackId& trackId)
{
    muse::mpe::OffStreamChanges offStream = playback->trackPlaybackData(trackId).offStream; // shares the channel
    offStream.send(muse::mpe::PlaybackEventsMap(), true);
}
}
