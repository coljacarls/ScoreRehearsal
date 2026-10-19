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

#include "rehearsalmixermodel.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <unordered_map>

#include <QJsonArray>
#include <QJsonObject>

#include "async/notifylist.h"
#include "log.h"
#include "translation.h"

#include "notation/imasternotation.h"
#include "notation/inotationparts.h"
#include "notation/inotationplayback.h"

#include "project/inotationproject.h"
#include "project/iprojectaudiosettings.h"
#include "project/types/projecturis.h"

#include "rehearsalsessionstore.h"

using namespace muse;
using namespace mu::appshell;
using namespace mu::playback;
using namespace muse::audio;
using namespace mu::engraving;
using namespace mu::notation;
using namespace mu::project;

static constexpr int INVALID_MIXER_INDEX = -1;

//! Rounds to 0.1 dB and clamps to the rehearsal fader range
static float normalizedMixerDb(float db)
{
    return std::clamp(std::round(db * 10.f) / 10.f, REHEARSAL_VOLUME_DB_MIN, REHEARSAL_VOLUME_DB_MAX);
}

static float normalizedAbsoluteDb(float db)
{
    return std::clamp(std::round(db * 10.f) / 10.f, REHEARSAL_VOLUME_DB_MIN, ABSOLUTE_VOLUME_DB_MAX);
}

//! Pans are kept in whole percent, so that the slider and the audio agree
static float normalizedPan(float pan)
{
    return std::clamp(std::round(pan * 100.f) / 100.f, BALANCE_MIN.raw(), BALANCE_MAX.raw());
}

//! The volume of a track as stored in the score
static float scoreVolumeDb(const INotationProjectPtr& project, const InstrumentTrackId& instrumentTrackId)
{
    const AudioOutputParams& params = project->audioSettings()->trackOutputParams(instrumentTrackId);
    return normalizedAbsoluteDb(params.control().volume.evaluateAt(muse::secs_t(0.0), VOLUME_DB_MIN, VOLUME_DB_MAX).raw());
}

//! The pan (balance) of a track as stored in the score, -1 (left)..1 (right)
static float scorePan(const INotationProjectPtr& project, const InstrumentTrackId& instrumentTrackId)
{
    const AudioOutputParams& params = project->audioSettings()->trackOutputParams(instrumentTrackId);
    return normalizedPan(params.control().balance.evaluateAt(muse::secs_t(0.0), BALANCE_MIN, BALANCE_MAX).raw());
}

namespace {
struct AbsoluteTrackMix {
    bool audible = true;
    float volumeDb = 0.f; // absolute volume the track is supposed to have
};

//! The faders are absolute: the volume of a track is its fader (plus the offset of its group), and the
//! accompaniment level lowers the tracks that are not solo while there is a solo
AbsoluteTrackMix absoluteTrackMix(const RehearsalTrackState& track, bool anySolo, float accompanimentDb)
{
    const auto isSilent = [](float db) { return db <= REHEARSAL_VOLUME_DB_MIN; };

    const float faderDb = std::clamp(track.volumeDb, REHEARSAL_VOLUME_DB_MIN, ABSOLUTE_VOLUME_DB_MAX);
    const bool isAccompaniment = anySolo && !track.isSolo;
    const float accompaniment = std::clamp(accompanimentDb, REHEARSAL_VOLUME_DB_MIN, REHEARSAL_VOLUME_DB_MAX);

    const float volumeDb = faderDb + (isAccompaniment ? accompaniment : 0.f);

    AbsoluteTrackMix mix;
    mix.audible = track.isOn && !isSilent(faderDb) && !(isAccompaniment && isSilent(accompaniment)) && !isSilent(volumeDb);
    mix.volumeDb = std::max(volumeDb, REHEARSAL_VOLUME_DB_MIN);
    return mix;
}
}

//! Track names come XML-escaped from the score (e.g. "F Horn 1 &amp; 3"); the faders show plain text
static QString plainTrackTitle(QString title)
{
    title.replace(QStringLiteral("&lt;"), QStringLiteral("<"));
    title.replace(QStringLiteral("&gt;"), QStringLiteral(">"));
    title.replace(QStringLiteral("&quot;"), QStringLiteral("\""));
    title.replace(QStringLiteral("&apos;"), QStringLiteral("'"));
    title.replace(QStringLiteral("&#39;"), QStringLiteral("'"));
    title.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
    return title;
}

namespace {
//! Lives as long as the project is open, independent of the page (and this model) being loaded
struct RehearsalMixerSession {
    std::weak_ptr<INotationProject> project;
    std::unordered_map<InstrumentTrackId, RehearsalTrackState> tracks;
    std::vector<FaderGroup> groups;
    int nextGroupId = 1;
    float accompanimentDb = REHEARSAL_VOLUME_DB_MIN; // silent the first time in a session
    std::optional<float> metronomeDb; // unset until the user moves the fader
    bool shownPartOnly = false; // play only the tracks of the shown part
    //! Track faders of sessions from before the faders were absolute: offsets to the volume in the score
    std::unordered_map<InstrumentTrackId, float> legacyRelativeDb;
    //! Pan of the tracks that were set once, -1 (left)..1 (right)
    std::unordered_map<InstrumentTrackId, float> pans;
};

QString trackKey(const InstrumentTrackId& id)
{
    return QString::fromStdString(id.partId.toStdString()) + QLatin1Char('|') + id.instrumentId.toQString();
}

std::optional<InstrumentTrackId> trackIdFromKey(const QString& key)
{
    const int separator = key.indexOf(QLatin1Char('|'));
    if (separator <= 0) {
        return std::nullopt;
    }

    bool ok = false;
    const quint64 partId = key.left(separator).toULongLong(&ok);
    if (!ok) {
        return std::nullopt;
    }

    return InstrumentTrackId { muse::ID(static_cast<uint64_t>(partId)), String::fromQString(key.mid(separator + 1)) };
}

QJsonObject mixerToJson(const RehearsalMixerSession& session)
{
    QJsonObject tracks;
    for (const auto& [id, state] : session.tracks) {
        QJsonObject track {
            { "on", state.isOn },
            { "solo", state.isSolo },
            { "absoluteDb", static_cast<double>(state.volumeDb) }
        };

        auto pan = session.pans.find(id);
        if (pan != session.pans.end()) {
            track["pan"] = static_cast<double>(pan->second);
        }

        tracks[trackKey(id)] = track;
    }

    QJsonArray groups;
    for (const FaderGroup& group : session.groups) {
        QJsonArray members;
        for (const InstrumentTrackId& member : group.members) {
            members.append(trackKey(member));
        }

        groups.append(QJsonObject {
            { "id", group.id },
            { "name", group.name },
            { "on", group.isOn },
            { "solo", group.isSolo },
            { "volumeDb", static_cast<double>(group.volumeDb) },
            { "expanded", group.isExpanded },
            { "members", members }
        });
    }

    QJsonObject result {
        { "accompanimentDb", static_cast<double>(session.accompanimentDb) },
        { "nextGroupId", session.nextGroupId },
        { "shownPartOnly", session.shownPartOnly },
        { "tracks", tracks },
        { "groups", groups }
    };

    if (session.metronomeDb) {
        result["metronomeDb"] = static_cast<double>(*session.metronomeDb);
    }

    return result;
}

void mixerFromJson(const QJsonObject& json, RehearsalMixerSession& session)
{
    if (json.isEmpty()) {
        return;
    }

    session.accompanimentDb = normalizedMixerDb(static_cast<float>(json.value("accompanimentDb").toDouble(REHEARSAL_VOLUME_DB_MIN)));
    session.nextGroupId = std::max(1, json.value("nextGroupId").toInt(1));
    session.shownPartOnly = json.value("shownPartOnly").toBool(false);

    if (json.contains("metronomeDb")) {
        session.metronomeDb = normalizedAbsoluteDb(static_cast<float>(json.value("metronomeDb").toDouble()));
    }

    const QJsonObject tracks = json.value("tracks").toObject();
    for (auto it = tracks.begin(); it != tracks.end(); ++it) {
        const std::optional<InstrumentTrackId> id = trackIdFromKey(it.key());
        if (!id) {
            continue;
        }

        const QJsonObject o = it.value().toObject();
        RehearsalTrackState state;
        state.instrumentTrackId = *id;
        state.isOn = o.value("on").toBool(true);
        state.isSolo = o.value("solo").toBool(false);
        if (o.contains("absoluteDb")) {
            state.volumeDb = normalizedAbsoluteDb(static_cast<float>(o.value("absoluteDb").toDouble()));
        } else {
            //! Made before the faders were absolute: converted when the track is known
            state.volumeDb = normalizedMixerDb(static_cast<float>(o.value("volumeDb").toDouble(REHEARSAL_VOLUME_DB_MAX)));
            session.legacyRelativeDb[*id] = state.volumeDb;
        }
        session.tracks[*id] = state;

        if (o.contains("pan")) {
            session.pans[*id] = normalizedPan(static_cast<float>(o.value("pan").toDouble()));
        }
    }

    for (const QJsonValue& value : json.value("groups").toArray()) {
        const QJsonObject o = value.toObject();

        FaderGroup group;
        group.id = o.value("id").toInt();
        group.name = o.value("name").toString();
        group.isOn = o.value("on").toBool(true);
        group.isSolo = o.value("solo").toBool(false);
        group.volumeDb = normalizedMixerDb(static_cast<float>(o.value("volumeDb").toDouble(REHEARSAL_VOLUME_DB_MAX)));
        group.isExpanded = o.value("expanded").toBool(false);

        for (const QJsonValue& member : o.value("members").toArray()) {
            if (const std::optional<InstrumentTrackId> id = trackIdFromKey(member.toString())) {
                group.members.push_back(*id);
            }
        }

        if (!group.name.isEmpty() && !group.members.empty()) {
            session.nextGroupId = std::max(session.nextGroupId, group.id + 1);
            session.groups.push_back(std::move(group));
        }
    }
}

//! The in-memory state of the open project, loaded from the stored session of its file
RehearsalMixerSession& mixerSessionFor(const INotationProjectPtr& project)
{
    static std::vector<RehearsalMixerSession> s_sessions;

    std::erase_if(s_sessions, [](const RehearsalMixerSession& session) {
        return session.project.expired();
    });

    for (RehearsalMixerSession& session : s_sessions) {
        if (session.project.lock() == project) {
            return session;
        }
    }

    RehearsalMixerSession& session = s_sessions.emplace_back();
    session.project = project;

    if (project) {
        RehearsalSessionStore& store = RehearsalSessionStore::instance();
        const RehearsalSession& stored = store.ensureForPath(project->path().toQString(), project->displayName());
        mixerFromJson(stored.mixer, session);
    }

    return session;
}
}

RehearsalMixerModel::RehearsalMixerModel(QObject* parent)
    : QAbstractListModel(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

RehearsalMixerModel::~RehearsalMixerModel()
{
    if (m_overrideApplied && controller()) {
        controller()->clearSessionMixOverride();
    }
}

void RehearsalMixerModel::componentComplete()
{
    controller()->playbackInitedChanged().onReceive(this, [this](bool) {
        reload();
        updateMixOverride();
    });

    controller()->trackAdded().onReceive(this, [this](const TrackId trackId) {
        onTrackAdded(trackId);
    });

    controller()->trackRemoved().onReceive(this, [this](const TrackId trackId) {
        onTrackRemoved(trackId);
    });

    context()->currentProjectChanged().onNotify(this, [this]() {
        reload();
        updateMixOverride();
    });

    context()->currentNotationChanged().onNotify(this, [this]() {
        updateScope();
    });

    interactive()->currentUri().ch.onReceive(this, [this](const Uri&) {
        updateMixOverride();
    });

    reload();
    updateMixOverride();
}

QVariant RehearsalMixerModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || !isValidRow(index.row())) {
        return QVariant();
    }

    const Row& row = m_rows.at(index.row());
    const Track* track = row.trackIndex != INVALID_MIXER_INDEX ? &m_tracks.at(row.trackIndex) : nullptr;
    const FaderGroup* group = row.groupIndex != INVALID_MIXER_INDEX ? &m_groups.at(row.groupIndex) : nullptr;

    switch (role) {
    case RowTypeRole:
        switch (row.type) {
        case RowType::Track: return QStringLiteral("track");
        case RowType::Group: return QStringLiteral("group");
        case RowType::Member: return QStringLiteral("member");
        case RowType::Footer: return QStringLiteral("footer");
        }
        break;
    case TitleRole:
        if (row.type == RowType::Group || row.type == RowType::Footer) {
            return group->name;
        }
        return track->title;
    case IsOnRole:
        if (track) {
            return track->state.isOn;
        }
        return group->isOn;
    case IsSoloRole:
        if (track) {
            return track->state.isSolo;
        }
        return group->isSolo;
    case VolumeDbRole:
        if (track) {
            return track->state.volumeDb;
        }
        return group->volumeDb;
    case EffectiveDbRole:
        return track ? effectiveDb(row.trackIndex) : REHEARSAL_VOLUME_DB_MIN;
    case ScoreDbRole:
        return track ? track->scoreDb : 0.0;
    case PanRole:
        return track ? static_cast<int>(std::lround(track->pan * 100.f)) : 0;
    case ScorePanRole:
        return track ? static_cast<int>(std::lround(track->scorePan * 100.f)) : 0;
    case GroupOnRole:
        return group ? group->isOn : true;
    case GroupSoloRole:
        return group ? group->isSolo : false;
    case IsExpandedRole:
        return group ? group->isExpanded : false;
    case MemberCountRole: {
        if (!group) {
            return 0;
        }
        int count = 0;
        for (const InstrumentTrackId& member : group->members) {
            if (indexOfTrack(member) != INVALID_MIXER_INDEX && isInScope(member)) {
                ++count;
            }
        }
        return count;
    }
    }

    return QVariant();
}

int RehearsalMixerModel::rowCount(const QModelIndex&) const
{
    return static_cast<int>(m_rows.size());
}

QHash<int, QByteArray> RehearsalMixerModel::roleNames() const
{
    static const QHash<int, QByteArray> roles {
        { RowTypeRole, "rowType" },
        { TitleRole, "title" },
        { IsOnRole, "isOn" },
        { IsSoloRole, "isSolo" },
        { VolumeDbRole, "volumeDb" },
        { EffectiveDbRole, "effectiveDb" },
        { ScoreDbRole, "scoreDb" },
        { PanRole, "pan" },
        { ScorePanRole, "scorePan" },
        { GroupOnRole, "groupOn" },
        { GroupSoloRole, "groupSolo" },
        { IsExpandedRole, "isExpanded" },
        { MemberCountRole, "memberCount" }
    };

    return roles;
}

bool RehearsalMixerModel::hasSolo() const
{
    return m_hasSolo;
}

float RehearsalMixerModel::accompanimentDb() const
{
    return m_accompanimentDb;
}

void RehearsalMixerModel::setAccompanimentDb(float db)
{
    db = normalizedMixerDb(db);
    if (m_accompanimentDb == db) {
        return;
    }

    m_accompanimentDb = db;
    emit accompanimentDbChanged();

    onValuesChanged();
}

float RehearsalMixerModel::metronomeDb() const
{
    return m_metronomeDb;
}

void RehearsalMixerModel::setMetronomeDb(float db)
{
    db = normalizedAbsoluteDb(db);
    if (m_metronomeDb == db) {
        return;
    }

    m_metronomeDb = db;
    emit metronomeDbChanged();

    if (const INotationProjectPtr project = context()->currentProject()) {
        mixerSessionFor(project).metronomeDb = db;
    }

    applyMetronomeDb();
    saveSession();
}

std::optional<TrackId> RehearsalMixerModel::metronomeTrackId() const
{
    const INotationProjectPtr project = context()->currentProject();
    if (!project || !controller()->isPlaybackInited()) {
        return std::nullopt;
    }

    const IPlaybackController::InstrumentTrackIdMap& instrumentTracks = controller()->instrumentTrackIdMap();
    auto it = instrumentTracks.find(project->masterNotation()->playback()->metronomeTrackId());
    if (it == instrumentTracks.cend()) {
        return std::nullopt;
    }

    return it->second;
}

void RehearsalMixerModel::loadMetronome()
{
    const std::optional<TrackId> trackId = metronomeTrackId();
    const INotationProjectPtr project = context()->currentProject();
    if (!trackId || !project) {
        return;
    }

    RehearsalMixerSession& session = mixerSessionFor(project);
    m_metronomeScoreDb = scoreVolumeDb(project, project->masterNotation()->playback()->metronomeTrackId());

    if (session.metronomeDb) {
        //! The volume of the session replaces the one of the score, also when the track is created again
        m_metronomeDb = *session.metronomeDb;
        applyMetronomeDb();
    } else {
        //! First time: the fader starts at the volume of the score
        m_metronomeDb = m_metronomeScoreDb;
        session.metronomeDb = m_metronomeDb;
        saveSession();
    }

    emit metronomeDbChanged();
}

void RehearsalMixerModel::applyMetronomeDb()
{
    const std::optional<TrackId> trackId = metronomeTrackId();
    if (!trackId) {
        return;
    }

    const float db = m_metronomeDb;
    playback()->params(*trackId).onResolve(this, [this, trackId, db](const TrackParams& params) {
        ControlParams control = params.control;
        control.volume = volume_db_t::make(db);
        control.muted = params.control.muted;
        playback()->setControlParams(*trackId, control);
    });
}

float RehearsalMixerModel::maxAbsoluteDb() const
{
    return ABSOLUTE_VOLUME_DB_MAX;
}

float RehearsalMixerModel::metronomeScoreDb() const
{
    return m_metronomeScoreDb;
}

float RehearsalMixerModel::minVolumeDb() const
{
    return REHEARSAL_VOLUME_DB_MIN;
}

float RehearsalMixerModel::maxVolumeDb() const
{
    return REHEARSAL_VOLUME_DB_MAX;
}

int RehearsalMixerModel::ungroupedTrackCount() const
{
    int count = 0;
    for (const Track& track : m_tracks) {
        if (isInScope(track.state.instrumentTrackId) && groupIndexOfTrack(track.state.instrumentTrackId) == INVALID_MIXER_INDEX) {
            ++count;
        }
    }

    return count;
}

bool RehearsalMixerModel::isPartShown() const
{
    return m_isPartShown;
}

bool RehearsalMixerModel::shownPartOnly() const
{
    return m_shownPartOnly;
}

bool RehearsalMixerModel::partScopeActive() const
{
    return m_shownPartOnly && m_isPartShown;
}

bool RehearsalMixerModel::isInScope(const InstrumentTrackId& instrumentTrackId) const
{
    return !partScopeActive() || std::find(m_shownPartIds.cbegin(), m_shownPartIds.cend(), instrumentTrackId.partId) != m_shownPartIds.cend();
}

bool RehearsalMixerModel::readShownPart()
{
    bool shown = false;
    std::vector<muse::ID> partIds;

    const INotationPtr current = context()->currentNotation();
    const IMasterNotationPtr master = context()->currentMasterNotation();
    if (current && master && current != master->notation() && current->parts()) {
        shown = true;

        async::NotifyList<const Part*> partList = current->parts()->partList();
        for (const Part* part : partList) {
            partIds.push_back(part->id());
        }
    }

    const bool changed = shown != m_isPartShown || partIds != m_shownPartIds;
    m_isPartShown = shown;
    m_shownPartIds = std::move(partIds);
    return changed;
}

//! Another part (or the full score) is shown: the faders and the sound follow when only the shown part is played
void RehearsalMixerModel::updateScope()
{
    if (!readShownPart()) {
        return;
    }

    emit scopeChanged();

    if (!m_shownPartOnly) {
        return;
    }

    rebuildRows();
    updateHasSolo();
    updateMixOverride();
}

void RehearsalMixerModel::setShownPartOnly(bool only)
{
    if (m_shownPartOnly == only) {
        return;
    }

    m_shownPartOnly = only;

    if (const INotationProjectPtr project = context()->currentProject()) {
        mixerSessionFor(project).shownPartOnly = only;
    }

    emit scopeChanged();

    rebuildRows();
    updateHasSolo();
    saveSession();
    updateMixOverride();
}

void RehearsalMixerModel::setOn(int row, bool on)
{
    if (!isValidRow(row) || m_rows.at(row).type == RowType::Footer) {
        return;
    }

    const Row& r = m_rows.at(row);
    bool& value = r.type == RowType::Group ? m_groups[r.groupIndex].isOn : m_tracks[r.trackIndex].state.isOn;
    if (value == on) {
        return;
    }

    value = on;
    onValuesChanged();
}

void RehearsalMixerModel::setSolo(int row, bool solo)
{
    if (!isValidRow(row) || m_rows.at(row).type == RowType::Footer) {
        return;
    }

    const Row& r = m_rows.at(row);
    bool& value = r.type == RowType::Group ? m_groups[r.groupIndex].isSolo : m_tracks[r.trackIndex].state.isSolo;
    if (value == solo) {
        return;
    }

    value = solo;
    onValuesChanged();
}

void RehearsalMixerModel::setVolumeDb(int row, float db)
{
    if (!isValidRow(row) || m_rows.at(row).type == RowType::Footer) {
        return;
    }

    const Row& r = m_rows.at(row);
    //! The faders of the tracks are absolute, the ones of the groups are offsets
    db = r.type == RowType::Group ? normalizedMixerDb(db) : normalizedAbsoluteDb(db);

    float& value = r.type == RowType::Group ? m_groups[r.groupIndex].volumeDb : m_tracks[r.trackIndex].state.volumeDb;
    if (value == db) {
        return;
    }

    value = db;
    onValuesChanged();
}

void RehearsalMixerModel::setPan(int row, int percent)
{
    if (!isValidRow(row)) {
        return;
    }

    const Row& r = m_rows.at(row);
    if (r.trackIndex == INVALID_MIXER_INDEX || r.type == RowType::Group || r.type == RowType::Footer) {
        return;
    }

    const float pan = normalizedPan(static_cast<float>(percent) / 100.f);
    float& value = m_tracks[r.trackIndex].pan;
    if (value == pan) {
        return;
    }

    value = pan;
    onValuesChanged();
}

//! Every track is in the middle of the stereo image
void RehearsalMixerModel::setAllPanCentered()
{
    bool changed = false;

    for (Track& track : m_tracks) {
        if (track.pan != 0.f) {
            track.pan = 0.f;
            changed = true;
        }
    }

    if (changed) {
        onValuesChanged();
    }
}

//! Every track sounds: nothing is muted and there is no solo that silences the others
void RehearsalMixerModel::setAllOn()
{
    bool changed = false;

    for (Track& track : m_tracks) {
        if (!track.state.isOn) {
            track.state.isOn = true;
            changed = true;
        }

        if (track.state.isSolo) {
            track.state.isSolo = false;
            changed = true;
        }
    }

    for (FaderGroup& group : m_groups) {
        if (!group.isOn) {
            group.isOn = true;
            changed = true;
        }

        if (group.isSolo) {
            group.isSolo = false;
            changed = true;
        }
    }

    if (changed) {
        onValuesChanged();
    }
}

void RehearsalMixerModel::toggleExpanded(int row)
{
    if (!isValidRow(row) || m_rows.at(row).groupIndex == INVALID_MIXER_INDEX) {
        return;
    }

    const int groupIndex = m_rows.at(row).groupIndex;
    FaderGroup& group = m_groups[groupIndex];
    group.isExpanded = !group.isExpanded;

    saveSession();

    //! Insert or remove only the member and footer rows, so that the other rows
    //! (and the keyboard focus on the group row) are kept
    int groupRow = INVALID_MIXER_INDEX;
    for (size_t i = 0; i < m_rows.size(); ++i) {
        if (m_rows[i].type == RowType::Group && m_rows[i].groupIndex == groupIndex) {
            groupRow = static_cast<int>(i);
            break;
        }
    }

    std::vector<Row> rows = buildRows();
    const int diff = static_cast<int>(rows.size()) - static_cast<int>(m_rows.size());

    if (groupRow == INVALID_MIXER_INDEX || diff == 0) {
        rebuildRows();
        return;
    }

    if (diff > 0) {
        beginInsertRows(QModelIndex(), groupRow + 1, groupRow + diff);
        m_rows = std::move(rows);
        endInsertRows();
    } else {
        beginRemoveRows(QModelIndex(), groupRow + 1, groupRow - diff);
        m_rows = std::move(rows);
        endRemoveRows();
    }

    emit dataChanged(index(groupRow), index(groupRow), { IsExpandedRole });
    emit structureChanged();
}

QVariantList RehearsalMixerModel::ungroupedTracks() const
{
    QVariantList result;

    for (size_t i = 0; i < m_tracks.size(); ++i) {
        const Track& track = m_tracks.at(i);
        if (!isInScope(track.state.instrumentTrackId) || groupIndexOfTrack(track.state.instrumentTrackId) != INVALID_MIXER_INDEX) {
            continue;
        }

        QVariantMap item;
        item["trackKey"] = static_cast<int>(i);
        item["title"] = track.title;
        result << item;
    }

    return result;
}

QString RehearsalMixerModel::suggestedGroupName() const
{
    for (int number = 1;; ++number) {
        //: Default name of a new fader group, %1 is a number
        const QString name = qtrc("appshell", "Group %1").arg(number);

        const bool exists = std::any_of(m_groups.cbegin(), m_groups.cend(), [&name](const FaderGroup& group) {
            return group.name == name;
        });

        if (!exists) {
            return name;
        }
    }
}

bool RehearsalMixerModel::createGroup(const QString& name, const QVariantList& trackKeys)
{
    const QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty()) {
        return false;
    }

    std::vector<int> trackIndexes;
    for (const QVariant& key : trackKeys) {
        const int trackIndex = key.toInt();
        if (trackIndex < 0 || trackIndex >= static_cast<int>(m_tracks.size())) {
            continue;
        }

        if (groupIndexOfTrack(m_tracks.at(trackIndex).state.instrumentTrackId) != INVALID_MIXER_INDEX) {
            continue;
        }

        if (std::find(trackIndexes.cbegin(), trackIndexes.cend(), trackIndex) == trackIndexes.cend()) {
            trackIndexes.push_back(trackIndex);
        }
    }

    if (trackIndexes.empty()) {
        return false;
    }

    //! Keep the score order of the tracks
    std::sort(trackIndexes.begin(), trackIndexes.end());

    FaderGroup group;
    group.id = m_nextGroupId++;
    group.name = trimmedName;
    for (int trackIndex : trackIndexes) {
        group.members.push_back(m_tracks.at(trackIndex).state.instrumentTrackId);
    }

    m_groups.push_back(std::move(group));

    saveSession();
    rebuildRows();
    updateHasSolo();
    updateMixOverride();

    return true;
}

void RehearsalMixerModel::dissolveGroup(int row)
{
    if (!isValidRow(row) || m_rows.at(row).groupIndex == INVALID_MIXER_INDEX) {
        return;
    }

    //! The members keep their own settings, only the group's influence is removed
    m_groups.erase(m_groups.begin() + m_rows.at(row).groupIndex);

    saveSession();
    rebuildRows();
    updateHasSolo();
    updateMixOverride();
}

void RehearsalMixerModel::reload()
{
    TRACEFUNC;

    m_tracks.clear();
    m_groups.clear();
    m_nextGroupId = 1;
    m_shownPartOnly = false;

    const INotationProjectPtr project = context()->currentProject();
    float accompanimentDb = REHEARSAL_VOLUME_DB_MIN;

    m_sessionLoaded = false;

    if (project && controller()->isPlaybackInited()) {
        m_sessionLoaded = true;
        const RehearsalMixerSession& session = mixerSessionFor(project);
        accompanimentDb = session.accompanimentDb;
        m_groups = session.groups;
        m_nextGroupId = session.nextGroupId;
        m_shownPartOnly = session.shownPartOnly;

        const IPlaybackController::InstrumentTrackIdMap& instrumentTracks = controller()->instrumentTrackIdMap();

        for (const InstrumentTrackId& instrumentTrackId : orderedInstrumentTrackIds()) {
            auto it = instrumentTracks.find(instrumentTrackId);
            if (it != instrumentTracks.cend()) {
                m_tracks.push_back(makeTrack(instrumentTrackId, it->second));
            }
        }
    }

    readShownPart();
    rebuildRows();
    emit scopeChanged();

    if (m_accompanimentDb != accompanimentDb) {
        m_accompanimentDb = accompanimentDb;
        emit accompanimentDbChanged();
    }

    updateHasSolo();

    for (const Track& track : m_tracks) {
        requestTitle(track.trackId);
    }

    loadMetronome();
    saveSession();
}

void RehearsalMixerModel::onTrackAdded(const TrackId trackId)
{
    if (trackId == metronomeTrackId()) {
        loadMetronome();
        return;
    }

    if (indexOfTrack(trackId) != INVALID_MIXER_INDEX) {
        return;
    }

    const IPlaybackController::InstrumentTrackIdMap& instrumentTracks = controller()->instrumentTrackIdMap();
    auto instrumentIt = std::find_if(instrumentTracks.cbegin(), instrumentTracks.cend(), [trackId](const auto& pair) {
        return pair.second == trackId;
    });

    if (instrumentIt == instrumentTracks.cend()) {
        return;
    }

    const std::vector<InstrumentTrackId> ordered = orderedInstrumentTrackIds();
    auto position = std::find(ordered.cbegin(), ordered.cend(), instrumentIt->first);
    if (position == ordered.cend()) {
        return;
    }

    size_t insertIndex = 0;
    for (const Track& track : m_tracks) {
        if (std::find(ordered.cbegin(), position, track.state.instrumentTrackId) != position) {
            ++insertIndex;
        }
    }

    m_tracks.insert(m_tracks.begin() + insertIndex, makeTrack(instrumentIt->first, trackId));
    rebuildRows();
    saveSession();

    requestTitle(trackId);

    updateHasSolo();
    updateMixOverride();
}

void RehearsalMixerModel::onTrackRemoved(const TrackId trackId)
{
    const int trackIndex = indexOfTrack(trackId);
    if (trackIndex == INVALID_MIXER_INDEX) {
        return;
    }

    //! Group memberships stay in the session: the track may come back
    m_tracks.erase(m_tracks.begin() + trackIndex);
    rebuildRows();

    updateHasSolo();
    updateMixOverride();
}

void RehearsalMixerModel::requestTitle(const TrackId trackId)
{
    playback()->trackName(trackId)
    .onResolve(this, [this, trackId](const RetVal<TrackName>& trackName) {
        if (!trackName.ret) {
            LOGE() << "unable to get track name, error: " << trackName.ret.toString();
            return;
        }

        const int trackIndex = indexOfTrack(trackId);
        if (trackIndex == INVALID_MIXER_INDEX) {
            return;
        }

        m_tracks[trackIndex].title = plainTrackTitle(QString::fromStdString(trackName.val));

        for (size_t i = 0; i < m_rows.size(); ++i) {
            if (m_rows[i].trackIndex == trackIndex) {
                emit dataChanged(index(static_cast<int>(i)), index(static_cast<int>(i)), { TitleRole });
            }
        }
    });
}

std::vector<InstrumentTrackId> RehearsalMixerModel::orderedInstrumentTrackIds() const
{
    std::vector<InstrumentTrackId> result;

    const INotationProjectPtr project = context()->currentProject();
    if (!project) {
        return result;
    }

    const INotationPlaybackPtr notationPlayback = project->masterNotation()->playback();
    const InstrumentTrackId& metronomeTrackId = notationPlayback->metronomeTrackId();

    async::NotifyList<const Part*> partList = project->masterNotation()->parts()->partList();
    for (const Part* part : partList) {
        for (const InstrumentTrackId& instrumentTrackId : part->instrumentTrackIdList()) {
            if (instrumentTrackId != metronomeTrackId) {
                result.push_back(instrumentTrackId);
            }
        }
    }

    for (const auto& pair : controller()->instrumentTrackIdMap()) {
        if (notationPlayback->isChordSymbolsTrack(pair.first)) {
            result.push_back(pair.first);
        }
    }

    return result;
}

RehearsalMixerModel::Track RehearsalMixerModel::makeTrack(const InstrumentTrackId& instrumentTrackId, const TrackId trackId) const
{
    Track track;
    track.trackId = trackId;
    track.state.instrumentTrackId = instrumentTrackId;

    if (const INotationProjectPtr project = context()->currentProject()) {
        RehearsalMixerSession& session = mixerSessionFor(project);
        track.scoreDb = scoreVolumeDb(project, instrumentTrackId);
        track.state.volumeDb = track.scoreDb; // the faders start at the volume of the score
        track.scorePan = scorePan(project, instrumentTrackId);
        track.pan = track.scorePan; // ... and at the pan of the score

        auto panIt = session.pans.find(instrumentTrackId);
        if (panIt != session.pans.cend()) {
            track.pan = panIt->second;
        }

        auto it = session.tracks.find(instrumentTrackId);
        if (it != session.tracks.cend()) {
            track.state = it->second;

            auto legacy = session.legacyRelativeDb.find(instrumentTrackId);
            if (legacy != session.legacyRelativeDb.end()) {
                track.state.volumeDb = normalizedAbsoluteDb(track.scoreDb + legacy->second);
                session.legacyRelativeDb.erase(legacy);
            }
        }
    }

    return track;
}

int RehearsalMixerModel::indexOfTrack(const TrackId trackId) const
{
    for (size_t i = 0; i < m_tracks.size(); ++i) {
        if (m_tracks[i].trackId == trackId) {
            return static_cast<int>(i);
        }
    }

    return INVALID_MIXER_INDEX;
}

int RehearsalMixerModel::indexOfTrack(const InstrumentTrackId& instrumentTrackId) const
{
    for (size_t i = 0; i < m_tracks.size(); ++i) {
        if (m_tracks[i].state.instrumentTrackId == instrumentTrackId) {
            return static_cast<int>(i);
        }
    }

    return INVALID_MIXER_INDEX;
}

int RehearsalMixerModel::groupIndexOfTrack(const InstrumentTrackId& instrumentTrackId) const
{
    for (size_t i = 0; i < m_groups.size(); ++i) {
        const std::vector<InstrumentTrackId>& members = m_groups[i].members;
        if (std::find(members.cbegin(), members.cend(), instrumentTrackId) != members.cend()) {
            return static_cast<int>(i);
        }
    }

    return INVALID_MIXER_INDEX;
}

bool RehearsalMixerModel::isValidRow(int row) const
{
    return row >= 0 && row < rowCount();
}

void RehearsalMixerModel::rebuildRows()
{
    beginResetModel();
    m_rows = buildRows();
    endResetModel();

    emit structureChanged();
}

std::vector<RehearsalMixerModel::Row> RehearsalMixerModel::buildRows() const
{
    std::vector<Row> rows;
    std::vector<bool> groupAdded(m_groups.size(), false);

    //! A group takes the place of its first member present in the track list
    for (size_t trackIndex = 0; trackIndex < m_tracks.size(); ++trackIndex) {
        if (!isInScope(m_tracks[trackIndex].state.instrumentTrackId)) {
            continue;
        }

        const int groupIndex = groupIndexOfTrack(m_tracks[trackIndex].state.instrumentTrackId);

        if (groupIndex == INVALID_MIXER_INDEX) {
            rows.push_back({ RowType::Track, static_cast<int>(trackIndex), INVALID_MIXER_INDEX });
            continue;
        }

        if (groupAdded[groupIndex]) {
            continue;
        }

        groupAdded[groupIndex] = true;

        const FaderGroup& group = m_groups[groupIndex];
        rows.push_back({ RowType::Group, INVALID_MIXER_INDEX, groupIndex });

        if (!group.isExpanded) {
            continue;
        }

        for (const InstrumentTrackId& member : group.members) {
            const int memberIndex = indexOfTrack(member);
            if (memberIndex != INVALID_MIXER_INDEX && isInScope(member)) {
                rows.push_back({ RowType::Member, memberIndex, groupIndex });
            }
        }

        rows.push_back({ RowType::Footer, INVALID_MIXER_INDEX, groupIndex });
    }

    return rows;
}

std::vector<RehearsalTrackState> RehearsalMixerModel::effectiveTrackStates() const
{
    std::vector<RehearsalTrackState> states;
    states.reserve(m_tracks.size());

    for (const Track& track : m_tracks) {
        states.push_back(track.state);
    }

    return applyFaderGroups(states, m_groups);
}

float RehearsalMixerModel::effectiveDb(int trackIndex) const
{
    const Track& track = m_tracks.at(trackIndex);
    const int groupIndex = groupIndexOfTrack(track.state.instrumentTrackId);

    const RehearsalTrackState state = groupIndex == INVALID_MIXER_INDEX
                                      ? track.state
                                      : applyFaderGroup(track.state, m_groups.at(groupIndex));

    const AbsoluteTrackMix mix = absoluteTrackMix(state, m_hasSolo, m_accompanimentDb);
    return mix.audible ? mix.volumeDb : REHEARSAL_VOLUME_DB_MIN;
}

void RehearsalMixerModel::onValuesChanged()
{
    updateHasSolo();

    if (!m_rows.empty()) {
        emit dataChanged(index(0), index(rowCount() - 1), {
            IsOnRole, IsSoloRole, VolumeDbRole, EffectiveDbRole, GroupOnRole, GroupSoloRole, PanRole
        });
    }

    saveSession();
    updateMixOverride();
}

QStringList RehearsalMixerModel::soloPartIds() const
{
    return m_soloPartIds;
}

void RehearsalMixerModel::updateHasSolo()
{
    //! A solo of a track that is not played (another part is shown) does not count
    std::vector<RehearsalTrackState> states;
    for (const RehearsalTrackState& state : effectiveTrackStates()) {
        if (isInScope(state.instrumentTrackId)) {
            states.push_back(state);
        }
    }

    QStringList soloPartIds;
    for (const RehearsalTrackState& state : states) {
        if (!state.isSolo) {
            continue;
        }

        const QString partId = QString::fromStdString(state.instrumentTrackId.partId.toStdString());
        if (!soloPartIds.contains(partId)) {
            soloPartIds << partId;
        }
    }

    if (m_soloPartIds != soloPartIds) {
        m_soloPartIds = soloPartIds;
        emit soloPartIdsChanged();
    }

    const bool hasSolo = hasAnySolo(states);
    if (m_hasSolo == hasSolo) {
        return;
    }

    m_hasSolo = hasSolo;
    emit hasSoloChanged();

    if (!m_rows.empty()) {
        emit dataChanged(index(0), index(rowCount() - 1), { EffectiveDbRole });
    }
}

void RehearsalMixerModel::saveSession()
{
    //! NOTE An empty model (the playback is not ready yet) would wipe the groups and faders of the session
    const INotationProjectPtr project = context()->currentProject();
    if (!m_sessionLoaded || !project) {
        return;
    }

    RehearsalMixerSession& session = mixerSessionFor(project);
    session.accompanimentDb = m_accompanimentDb;
    session.groups = m_groups;
    session.nextGroupId = m_nextGroupId;

    for (const Track& track : m_tracks) {
        session.tracks[track.state.instrumentTrackId] = track.state;
        session.pans[track.state.instrumentTrackId] = track.pan;
    }

    RehearsalSessionStore& store = RehearsalSessionStore::instance();
    if (RehearsalSession* stored = store.findByPath(project->path().toQString())) {
        stored->mixer = mixerToJson(session);
        store.markChanged(stored->id);
    }
}

bool RehearsalMixerModel::isPageActive() const
{
    //! NOTE The page stays active while a dialog is opened on top of it
    const std::vector<Uri> stack = interactive()->stack();
    return !stack.empty() && stack.front() == NOTATION_REHEARSAL_PAGE_URI;
}

//! The mix is made of offsets to the volumes in the score, so that the volume of each track becomes its absolute one
SessionMix RehearsalMixerModel::calculateMix() const
{
    const std::vector<RehearsalTrackState> states = effectiveTrackStates();

    std::vector<RehearsalTrackState> playedStates;
    for (const RehearsalTrackState& state : states) {
        if (isInScope(state.instrumentTrackId)) {
            playedStates.push_back(state);
        }
    }

    const bool anySolo = hasAnySolo(playedStates);

    SessionMix result;
    for (size_t i = 0; i < states.size(); ++i) {
        //! The tracks of the parts that are not shown are not played
        if (!isInScope(states[i].instrumentTrackId)) {
            result[states[i].instrumentTrackId] = { false, 0.f };
            continue;
        }

        const AbsoluteTrackMix mix = absoluteTrackMix(states[i], anySolo, m_accompanimentDb);
        result[states[i].instrumentTrackId] = { mix.audible, mix.volumeDb - m_tracks[i].scoreDb, m_tracks[i].pan };
    }

    return result;
}

void RehearsalMixerModel::updateMixOverride()
{
    if (isPageActive() && context()->currentProject()) {
        controller()->setSessionMixOverride(calculateMix());
        m_overrideApplied = true;
        return;
    }

    if (m_overrideApplied) {
        controller()->clearSessionMixOverride();
        m_overrideApplied = false;
    }
}
