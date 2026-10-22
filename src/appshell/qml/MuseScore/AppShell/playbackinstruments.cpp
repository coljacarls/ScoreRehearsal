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

#include "playbackinstruments.h"

#include <algorithm>
#include <map>
#include <vector>

#include <QCollator>

#include "translation.h"

#include "audio/common/soundfonttypes.h"

#include "playback/qml/MuseScore/Playback/msbasicpresetscategories.h"

using namespace mu::appshell;
using namespace muse;
using namespace muse::audio;
using namespace muse::audio::synth;

namespace {
const String MS_BASIC_SOUNDFONT_NAME(u"MS Basic");

struct Node {
    QString title;
    QString subtitle;
    QString key;
    std::vector<Node> children;
};

Node& childNamed(Node& parent, const QString& title)
{
    for (Node& child : parent.children) {
        if (child.key.isEmpty() && child.title.compare(title, Qt::CaseInsensitive) == 0) {
            return child;
        }
    }

    parent.children.push_back(Node { title, QString(), QString(), {} });
    return parent.children.back();
}

void addInstrument(Node& parent, const AudioResourceMeta& meta, const QString& title, const QString& subtitle)
{
    parent.children.push_back(Node { title, subtitle, playbackInstrumentKey(meta), {} });
}

void sortNodes(Node& node)
{
    static QCollator collator = [] {
        QCollator c;
        c.setNumericMode(true);
        c.setCaseSensitivity(Qt::CaseInsensitive);
        return c;
    }();

    std::stable_sort(node.children.begin(), node.children.end(), [](const Node& a, const Node& b) {
        return collator.compare(a.title, b.title) < 0;
    });

    for (Node& child : node.children) {
        sortNodes(child);
    }
}

using ProgramMap = std::map<std::pair<int, int>, const AudioResourceMeta*>;

void addMsBasicItem(Node& parent, const mu::playback::MsBasicItem& item, const ProgramMap& presets)
{
    if (!item.subItems.empty()) {
        Node& category = childNamed(parent, item.title.toQString());
        for (const mu::playback::MsBasicItem& subItem : item.subItems) {
            addMsBasicItem(category, subItem, presets);
        }
        return;
    }

    auto it = presets.find({ item.preset.bank, item.preset.program });
    if (it == presets.cend()) {
        return;
    }

    const AudioResourceMeta& meta = *it->second;

    QString name = meta.attributeVal(PRESET_NAME_ATTRIBUTE).toQString();
    if (name.isEmpty()) {
        name = qtrc("appshell", "Bank %1, preset %2").arg(item.preset.bank).arg(item.preset.program);
    }

    //! The expressive variants are not usable (see the sound menu of MuseScore)
    if (name.contains(QStringLiteral("Expr."))) {
        return;
    }

    addInstrument(parent, meta, name, MS_BASIC_SOUNDFONT_NAME.toQString());
}

QVariantMap toVariant(const Node& node, PlaybackInstrumentCatalog& catalog, QList<int>& path)
{
    QVariantList children;
    for (const Node& child : node.children) {
        //! Categories without an instrument are left out
        if (child.key.isEmpty() && child.children.empty()) {
            continue;
        }

        path.push_back(static_cast<int>(children.size()));
        children << toVariant(child, catalog, path);
        path.pop_back();
    }

    QVariantMap result {
        { "title", node.title },
        { "subtitle", node.subtitle },
        { "key", node.key },
        { "children", children }
    };

    if (!node.key.isEmpty()) {
        catalog.items[node.key] = QVariantMap { { "key", node.key }, { "title", node.title }, { "subtitle", node.subtitle } };
        if (!catalog.paths.contains(node.key)) {
            catalog.paths[node.key] = path;
        }
    }

    return result;
}
}

QString mu::appshell::playbackInstrumentKey(const AudioResourceMeta& meta)
{
    return QString::fromStdString(meta.type) + QLatin1Char('|') + QString::fromStdString(meta.vendor) + QLatin1Char('|')
           + QString::fromStdString(meta.id);
}

PlaybackInstrumentCatalog mu::appshell::buildPlaybackInstrumentCatalog(const AudioResourceMetaList& resources)
{
    PlaybackInstrumentCatalog catalog;

    Node root;
    Node soundFonts { qtrc("appshell", "SoundFonts"), QString(), QString(), {} };
    Node vstPlugins { qtrc("appshell", "VST"), QString(), QString(), {} };

    ProgramMap msBasicPresets;
    std::map<String, std::vector<const AudioResourceMeta*> > otherSoundFonts;

    for (const AudioResourceMeta& meta : resources) {
        const AudioResourceType type = resourceTypeFromString(meta.type);
        catalog.resources[playbackInstrumentKey(meta)] = meta;

        switch (type) {
        case AudioResourceType::MuseSamplerSoundPack: {
            QString category = meta.attributeVal(u"museCategory").toQString();
            if (category.isEmpty()) {
                category = qtrc("appshell", "Other");
            }

            QString pack = meta.attributeVal(u"musePack").toQString();
            if (pack.isEmpty()) {
                pack = meta.attributeVal(u"museVendorName").toQString();
            }

            addInstrument(childNamed(root, category), meta, meta.attributeVal(u"museName").toQString(), pack);
            break;
        }
        case AudioResourceType::FluidSoundfont: {
            bool bankOk = false, programOk = false;
            const int bank = meta.attributeVal(PRESET_BANK_ATTRIBUTE).toInt(&bankOk);
            const int program = meta.attributeVal(PRESET_PROGRAM_ATTRIBUTE).toInt(&programOk);
            if (!bankOk || !programOk) {
                break; // "Choose automatically"
            }

            const String& soundFont = meta.attributeVal(SOUNDFONT_NAME_ATTRIBUTE);
            if (soundFont == MS_BASIC_SOUNDFONT_NAME) {
                msBasicPresets[{ bank, program }] = &meta;
            } else {
                otherSoundFonts[soundFont].push_back(&meta);
            }
            break;
        }
        case AudioResourceType::VstPlugin: {
            const QString vendor = QString::fromStdString(meta.vendor);
            addInstrument(childNamed(vstPlugins, vendor), meta, QString::fromStdString(meta.id), vendor);
            break;
        }
        default:
            break;
        }
    }

    for (const mu::playback::MsBasicItem& category : mu::playback::MS_BASIC_PRESET_CATEGORIES) {
        addMsBasicItem(root, category, msBasicPresets);
    }

    for (const auto& [soundFont, presets] : otherSoundFonts) {
        Node& soundFontNode = childNamed(soundFonts, soundFont.toQString());

        for (const AudioResourceMeta* meta : presets) {
            const int bank = meta->attributeVal(PRESET_BANK_ATTRIBUTE).toInt();
            const int program = meta->attributeVal(PRESET_PROGRAM_ATTRIBUTE).toInt();

            QString name = meta->attributeVal(PRESET_NAME_ATTRIBUTE).toQString();
            if (name.isEmpty()) {
                name = qtrc("appshell", "Preset %1").arg(program);
            }

            addInstrument(childNamed(soundFontNode, qtrc("appshell", "Bank %1").arg(bank)), *meta, name, soundFont.toQString());
        }
    }

    sortNodes(root);
    sortNodes(soundFonts);
    sortNodes(vstPlugins);

    //! The categories come first, the other sources after them
    root.children.push_back(std::move(soundFonts));
    root.children.push_back(std::move(vstPlugins));

    QList<int> path;
    catalog.tree = toVariant(root, catalog, path).value("children").toList();

    return catalog;
}
