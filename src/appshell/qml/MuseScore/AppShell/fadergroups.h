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

#include <vector>

#include <QString>

#include "engraving/types/types.h"
#include "playback/internal/rehearsalmix.h"

namespace mu::appshell {
//! The track faders are absolute: they set the volume of the track itself, up to the maximum of the audio engine
constexpr float ABSOLUTE_VOLUME_DB_MAX = 12.f;

//! A fader group of the rehearsal page: one fader, mute and solo for several tracks
struct FaderGroup {
    int id = 0;
    QString name;
    bool isOn = true;
    bool isSolo = false;
    float volumeDb = playback::REHEARSAL_VOLUME_DB_MAX; // added to the member faders
    bool isExpanded = false;
    std::vector<engraving::InstrumentTrackId> members;
};

//! The member state as seen by the mix:
//! on = member on && group on; solo = member solo || group solo;
//! volume = member volume (absolute) + group volume (an offset), clamped to the absolute range
playback::RehearsalTrackState applyFaderGroup(const playback::RehearsalTrackState& member, const FaderGroup& group);

//! Applies each group to its members; tracks without a group are returned unchanged
std::vector<playback::RehearsalTrackState> applyFaderGroups(const std::vector<playback::RehearsalTrackState>& tracks,
                                                            const std::vector<FaderGroup>& groups);
}
