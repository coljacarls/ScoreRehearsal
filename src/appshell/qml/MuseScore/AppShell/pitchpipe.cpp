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

#include "pitchpipe.h"

#include <limits>
#include <vector>

#include "engraving/dom/chord.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/note.h"
#include "engraving/dom/score.h"
#include "engraving/dom/utils.h"

#include "offstreamplayback.h"

#include "notation/imasternotation.h"
#include "notation/inotation.h"
#include "notation/inotationelements.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationplayback.h"

using namespace mu::appshell;
using namespace mu::engraving;

namespace {
//! The notes a hit element (notehead, stem, accidental, dot, ...) stands for
std::vector<const Note*> notesOf(const EngravingItem* element)
{
    if (element->isNote()) {
        return { toNote(element) };
    }

    for (const EngravingObject* item = element; item; item = item->parent()) {
        if (item->isChord()) {
            std::vector<const Note*> notes;
            for (const Note* note : toChord(item)->notes()) {
                notes.push_back(note);
            }
            return notes;
        }

        if (item->isMeasure() || item->isSegment()) {
            break;
        }
    }

    return {};
}

//! The same note in the full score (the shown score can be a part)
const Note* masterNote(const Note* note, const MasterScore* master)
{
    if (note->score() == master) {
        return note;
    }

    for (const EngravingObject* linked : note->linkList()) {
        if (linked->score() == master && linked->isNote()) {
            return toNote(linked);
        }
    }

    return nullptr;
}
}

PitchPipe::PitchPipe(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

PitchPipe::~PitchPipe()
{
    if (m_active) {
        stopSound();
    }
}

bool PitchPipe::sounding() const
{
    return m_note != nullptr;
}

QRectF PitchPipe::noteRect() const
{
    return m_noteRect;
}

void PitchPipe::begin(const QPointF& logicPos)
{
    //! Notes of the off stream are only heard while the playback is not running
    if (playbackController()->isPlaying()) {
        playbackController()->pause();
    }

    m_active = true;
    playAt(logicPos);
}

void PitchPipe::move(const QPointF& logicPos)
{
    if (m_active) {
        playAt(logicPos);
    }
}

void PitchPipe::end()
{
    if (!m_active) {
        return;
    }

    m_active = false;
    stopSound();
}

const Note* PitchPipe::noteAt(const QPointF& logicPos) const
{
    const notation::INotationPtr notation = globalContext()->currentNotation();
    if (!notation || !notation->elements() || !notation->interaction()) {
        return nullptr;
    }

    //! A note head is about one staff space high and 1.3 wide; a finger covers a bit more
    const double radius = 0.8 * notation->elements()->msScore()->style().spatium();
    const muse::PointF pos = muse::PointF::fromQPointF(logicPos);

    const Note* best = nullptr;
    double bestDistance = std::numeric_limits<double>::max();
    for (const EngravingItem* hit : notation->interaction()->hitElements(pos, static_cast<float>(radius))) {
        for (const Note* note : notesOf(hit)) {
            const double distance = (note->canvasBoundingRect().center() - pos).manhattanLength();
            if (distance < bestDistance) {
                best = note;
                bestDistance = distance;
            }
        }
    }

    return best;
}

void PitchPipe::playAt(const QPointF& logicPos)
{
    const Note* note = noteAt(logicPos);
    if (note == m_note) {
        return;
    }

    stopSound();

    const notation::IMasterNotationPtr master = globalContext()->currentMasterNotation();
    if (!note || !master || !master->masterScore() || !master->playback()) {
        return;
    }

    const Note* playedNote = masterNote(note, master->masterScore());
    if (!playedNote) {
        return;
    }

    const Note* start = tieStart(playedNote);
    if (!start->play()) {
        return;
    }

    m_soundingTrack = makeInstrumentTrackId(start);
    m_hasSoundingTrack = true;
    master->playback()->triggerEventsForItems({ start }, HOLD_DURATION, true);

    m_note = note;
    m_noteRect = note->canvasBoundingRect().toQRectF();
    emit noteRectChanged();
}

void PitchPipe::stopSound()
{
    if (m_hasSoundingTrack) {
        const notation::IMasterNotationPtr master = globalContext()->currentMasterNotation();
        if (master && master->playback()) {
            silenceTrack(master->playback(), m_soundingTrack);
        }

        m_hasSoundingTrack = false;
    }

    if (m_note) {
        m_note = nullptr;
        m_noteRect = QRectF();
        emit noteRectChanged();
    }
}
