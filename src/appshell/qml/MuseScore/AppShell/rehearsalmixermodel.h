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

#include <QAbstractListModel>
#include <QQmlParserStatus>
#include <optional>

#include <QVariantList>
#include <qqmlintegration.h>

#include "modularity/ioc.h"
#include "async/asyncable.h"
#include "audio/main/iplayback.h"
#include "context/iglobalcontext.h"
#include "interactive/iinteractive.h"

#include "playback/iplaybackcontroller.h"
#include "playback/internal/rehearsalmix.h"

#include "fadergroups.h"

namespace mu::appshell {
//! Track faders of the rehearsal page with fader groups.
//! A group replaces its member faders by one group fader (volume, mute, solo) that acts on
//! all members; the members can still be adjusted individually when the group is expanded.
//! Flat row list: ungrouped tracks, group headers, expanded members and a footer per expanded group.
//! Session-only: nothing is written to the score.
class RehearsalMixerModel : public QAbstractListModel, public QQmlParserStatus, public muse::async::Asyncable,
    public muse::Contextable
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(bool hasSolo READ hasSolo NOTIFY hasSoloChanged)
    Q_PROPERTY(QStringList soloPartIds READ soloPartIds NOTIFY soloPartIdsChanged)
    Q_PROPERTY(float accompanimentDb READ accompanimentDb WRITE setAccompanimentDb NOTIFY accompanimentDbChanged)
    Q_PROPERTY(float metronomeDb READ metronomeDb WRITE setMetronomeDb NOTIFY metronomeDbChanged)
    Q_PROPERTY(float minVolumeDb READ minVolumeDb CONSTANT)
    Q_PROPERTY(float maxVolumeDb READ maxVolumeDb CONSTANT)
    Q_PROPERTY(int ungroupedTrackCount READ ungroupedTrackCount NOTIFY structureChanged)

    QML_ELEMENT

    muse::ContextInject<muse::audio::IPlayback> playback = { this };
    muse::ContextInject<playback::IPlaybackController> controller = { this };
    muse::ContextInject<context::IGlobalContext> context = { this };
    muse::ContextInject<muse::IInteractive> interactive = { this };

public:
    explicit RehearsalMixerModel(QObject* parent = nullptr);
    ~RehearsalMixerModel() override;

    enum Roles {
        RowTypeRole = Qt::UserRole + 1, // "rowType": "track" | "group" | "member" | "footer"
        TitleRole,                      // "title": track title or group name
        IsOnRole,                       // "isOn": own on/off state
        IsSoloRole,                     // "isSolo": own solo state
        VolumeDbRole,                   // "volumeDb": own fader, -60..0
        EffectiveDbRole,                // "effectiveDb": tracks/members: offset to the score mix, -60 = silent
        GroupOnRole,                    // "groupOn": members: whether the group is on
        GroupSoloRole,                  // "groupSolo": members: whether the group is solo
        IsExpandedRole,                 // "isExpanded": groups and their rows
        MemberCountRole                 // "memberCount": groups
    };

    QVariant data(const QModelIndex& index, int role) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool hasSolo() const;

    //! Ids of the parts that have a solo track (own or by group); empty without solo
    QStringList soloPartIds() const;

    float accompanimentDb() const;
    void setAccompanimentDb(float db);

    //! Volume of the metronome in dB (the metronome is not part of the rehearsal mix)
    float metronomeDb() const;
    void setMetronomeDb(float db);

    float minVolumeDb() const;
    float maxVolumeDb() const;

    int ungroupedTrackCount() const;

    Q_INVOKABLE void setOn(int row, bool on);
    Q_INVOKABLE void setSolo(int row, bool solo);
    Q_INVOKABLE void setVolumeDb(int row, float db);
    Q_INVOKABLE void setAllOn();

    Q_INVOKABLE void toggleExpanded(int row);

    //! Tracks that can be added to a new group: [{ trackKey: int, title: string }]
    Q_INVOKABLE QVariantList ungroupedTracks() const;
    Q_INVOKABLE QString suggestedGroupName() const;
    //! Returns false if the name is empty or no valid track is given
    Q_INVOKABLE bool createGroup(const QString& name, const QVariantList& trackKeys);
    Q_INVOKABLE void dissolveGroup(int row);

signals:
    void hasSoloChanged();
    void soloPartIdsChanged();
    void accompanimentDbChanged();
    void metronomeDbChanged();
    void structureChanged();

private:
    struct Track {
        playback::RehearsalTrackState state;
        muse::audio::TrackId trackId = -1;
        QString title;
    };

    enum class RowType {
        Track,
        Group,
        Member,
        Footer
    };

    struct Row {
        RowType type = RowType::Track;
        int trackIndex = -1; // Track, Member
        int groupIndex = -1; // Group, Member, Footer
    };

    void classBegin() override {}
    void componentComplete() override;

    void reload();
    void onTrackAdded(const muse::audio::TrackId trackId);
    void onTrackRemoved(const muse::audio::TrackId trackId);
    void requestTitle(const muse::audio::TrackId trackId);

    std::vector<engraving::InstrumentTrackId> orderedInstrumentTrackIds() const;
    Track makeTrack(const engraving::InstrumentTrackId& instrumentTrackId, const muse::audio::TrackId trackId) const;
    int indexOfTrack(const muse::audio::TrackId trackId) const;
    int indexOfTrack(const engraving::InstrumentTrackId& instrumentTrackId) const;
    int groupIndexOfTrack(const engraving::InstrumentTrackId& instrumentTrackId) const;
    bool isValidRow(int row) const;

    //! Rebuilds the row list from tracks and groups (resets the model)
    void rebuildRows();
    std::vector<Row> buildRows() const;

    std::vector<playback::RehearsalTrackState> effectiveTrackStates() const;
    float effectiveDb(int trackIndex) const;

    void onValuesChanged();
    void updateHasSolo();
    void saveSession();

    std::optional<muse::audio::TrackId> metronomeTrackId() const;
    //! Reads the volume of the metronome track, or applies the one of the session
    void loadMetronome();
    void applyMetronomeDb();

    bool isPageActive() const;
    void updateMixOverride();

    std::vector<Track> m_tracks;
    std::vector<FaderGroup> m_groups;
    std::vector<Row> m_rows;
    int m_nextGroupId = 1;
    float m_accompanimentDb = playback::REHEARSAL_VOLUME_DB_MIN;
    float m_metronomeDb = 0.f;
    bool m_hasSolo = false;
    QStringList m_soloPartIds;
    bool m_overrideApplied = false;
};
}
