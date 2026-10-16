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

#include "fadergroups.h"

#include <algorithm>

using namespace mu::appshell;
using namespace mu::playback;

RehearsalTrackState mu::appshell::applyFaderGroup(const RehearsalTrackState& member, const FaderGroup& group)
{
    RehearsalTrackState result = member;
    result.isOn = member.isOn && group.isOn;
    result.isSolo = member.isSolo || group.isSolo;
    result.volumeDb = std::clamp(member.volumeDb + group.volumeDb, REHEARSAL_VOLUME_DB_MIN, ABSOLUTE_VOLUME_DB_MAX);
    return result;
}

std::vector<RehearsalTrackState> mu::appshell::applyFaderGroups(const std::vector<RehearsalTrackState>& tracks,
                                                                const std::vector<FaderGroup>& groups)
{
    std::vector<RehearsalTrackState> result;
    result.reserve(tracks.size());

    for (const RehearsalTrackState& track : tracks) {
        auto group = std::find_if(groups.cbegin(), groups.cend(), [&track](const FaderGroup& g) {
            return std::find(g.members.cbegin(), g.members.cend(), track.instrumentTrackId) != g.members.cend();
        });

        result.push_back(group != groups.cend() ? applyFaderGroup(track, *group) : track);
    }

    return result;
}
