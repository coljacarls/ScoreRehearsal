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

#include <QHash>
#include <QList>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include "audio/common/audiotypes.h"

namespace mu::appshell {
//! Stable identifier of a playback instrument (a sound resource): type, vendor and id
QString playbackInstrumentKey(const muse::audio::AudioResourceMeta& meta);

//! The playback instruments the audio engine offers, as a category tree to drill down into
//! (Muse Sounds and MS Basic by category, other SoundFonts and VST plugins below their own entries).
struct PlaybackInstrumentCatalog {
    //! Nodes: { title, subtitle, key, children }; key is set for instruments only (the leaves)
    QVariantList tree;
    //! key -> { key, title, subtitle }
    QHash<QString, QVariantMap> items;
    //! key -> the resource to play with
    QHash<QString, muse::audio::AudioResourceMeta> resources;
    //! key -> the indexes to follow in the tree to reach the instrument
    QHash<QString, QList<int> > paths;

    bool isEmpty() const { return resources.isEmpty(); }
};

PlaybackInstrumentCatalog buildPlaybackInstrumentCatalog(const muse::audio::AudioResourceMetaList& resources);
}
