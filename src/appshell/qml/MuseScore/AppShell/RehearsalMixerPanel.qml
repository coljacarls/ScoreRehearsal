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

pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents
import MuseScore.AppShell
import MuseScore.Playback

//! Track faders of the rehearsal page (right side), with fader groups.
//! Adapted from MuseScore's RehearsalTracksPanel.
Rectangle {
    id: root

    property NavigationSection navigationSection: null
    property int navigationOrderStart: 1

    readonly property RehearsalMixerModel mixerModel: mixer

    //! The metronome fader is shown while the metronome is enabled
    property bool metronomeVisible: false

    //! The page opens the dialog to create a fader group
    signal createGroupRequested()

    color: ui.theme.backgroundSecondaryColor

    QtObject {
        id: prv

        readonly property int touchSize: 44
        readonly property int margins: 12
        readonly property int cardSpacing: 8
        readonly property int memberIndent: 24
        readonly property color goldColor: "#E0B23A"
        readonly property color goldBackgroundColor: Qt.rgba(0.878, 0.698, 0.227, 0.15)

        function dbText(db) {
            if (db <= mixer.minVolumeDb) {
                return "-" + "∞" // minus infinity unicode
            }

            var rounded = Math.round(db * 10) / 10

            //: Volume in decibels, %1 is a number
            return qsTrc("appshell", "%1 dB").arg(rounded === 0 ? "0" : rounded.toFixed(1))
        }
    }

    RehearsalMixerModel {
        id: mixer
    }

    NavigationPanel {
        id: headerNavPanel

        name: "RehearsalTracksHeader"
        section: root.navigationSection
        order: root.navigationOrderStart
        enabled: root.enabled && root.visible
        direction: NavigationPanel.Horizontal
        accessible.name: headerTitle.text
    }

    NavigationPanel {
        id: metronomeNavPanel

        name: "RehearsalMetronome"
        section: root.navigationSection
        order: root.navigationOrderStart + 1
        enabled: root.enabled && root.visible && root.metronomeVisible
        direction: NavigationPanel.Horizontal
        accessible.name: metronomeTitle.text

        //! Scrolls the fader into view when it gets the keyboard focus
        onActiveChanged: function(active) {
            if (active) {
                Utils.ensureContentVisible(flickable, Qt.rect(metronomeCard.x, metronomeCard.y, metronomeCard.width, metronomeCard.height), prv.cardSpacing)
            }
        }
    }

    NavigationPanel {
        id: accompanimentNavPanel

        name: "RehearsalAccompaniment"
        section: root.navigationSection
        order: root.navigationOrderStart + 2
        enabled: accompanimentRow.visible
        direction: NavigationPanel.Horizontal
        accessible.name: accompanimentTitle.text
    }

    Item {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: prv.margins
        anchors.rightMargin: prv.margins

        height: prv.touchSize + 2 * prv.cardSpacing

        StyledTextLabel {
            id: headerTitle

            anchors.left: parent.left
            anchors.right: newGroupButton.left
            anchors.rightMargin: prv.cardSpacing
            anchors.verticalCenter: parent.verticalCenter

            horizontalAlignment: Text.AlignLeft
            font: ui.theme.largeBodyBoldFont
            text: qsTrc("appshell", "Tracks")
        }

        FlatButton {
            id: newGroupButton

            anchors.right: allOnButton.left
            anchors.rightMargin: 6
            anchors.verticalCenter: parent.verticalCenter

            height: prv.touchSize
            minWidth: prv.touchSize

            icon: IconCode.PLUS
            text: qsTrc("appshell", "Group")
            orientation: Qt.Horizontal

            toolTipTitle: qsTrc("appshell", "New fader group")
            enabled: mixer.ungroupedTrackCount > 0

            navigation.name: "NewGroupButton"
            navigation.panel: headerNavPanel
            navigation.order: 1

            onClicked: {
                root.createGroupRequested()
            }
        }

        FlatButton {
            id: allOnButton

            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter

            height: prv.touchSize
            minWidth: prv.touchSize

            text: qsTrc("appshell", "All on")

            navigation.name: "AllOnButton"
            navigation.panel: headerNavPanel
            navigation.order: 2

            onClicked: {
                mixer.setAllOn()
            }
        }
    }

    Rectangle {
        id: accompanimentRow

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: prv.margins
        anchors.rightMargin: prv.margins

        height: visible ? accompanimentContent.height + 2 * prv.cardSpacing : 0
        visible: mixer.hasSolo

        radius: 6
        color: prv.goldBackgroundColor
        border.width: 1
        border.color: prv.goldColor

        Column {
            id: accompanimentContent

            anchors.top: parent.top
            anchors.topMargin: prv.cardSpacing
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: prv.margins
            anchors.rightMargin: prv.margins

            spacing: 0

            Item {
                width: parent.width
                height: 24

                StyledTextLabel {
                    id: accompanimentTitle

                    anchors.left: parent.left
                    anchors.right: accompanimentValue.left
                    anchors.verticalCenter: parent.verticalCenter

                    horizontalAlignment: Text.AlignLeft
                    font: ui.theme.bodyBoldFont
                    text: qsTrc("appshell", "Accompaniment")
                }

                StyledTextLabel {
                    id: accompanimentValue

                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter

                    font: ui.theme.bodyBoldFont
                    text: accompanimentSlider.valueText
                }
            }

            HorizontalVolumeSlider {
                id: accompanimentSlider

                width: parent.width

                from: mixer.minVolumeDb
                to: mixer.maxVolumeDb
                defaultDb: mixer.minVolumeDb
                volumeDb: mixer.accompanimentDb
                accentColor: prv.goldColor

                navigation.name: "AccompanimentSlider"
                navigation.panel: accompanimentNavPanel
                navigation.order: 1
                navigation.accessible.name: accompanimentTitle.text + " " + accompanimentValue.text

                onVolumeDbMoved: function(db) {
                    mixer.accompanimentDb = db
                }
            }
        }
    }

    StyledFlickable {
        id: flickable

        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: accompanimentRow.visible ? accompanimentRow.top : parent.bottom
        anchors.bottomMargin: accompanimentRow.visible ? prv.cardSpacing : 0

        contentHeight: rowsColumn.height + prv.cardSpacing

        Column {
            id: rowsColumn

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: prv.margins
            anchors.rightMargin: prv.margins

            spacing: prv.cardSpacing

            //! Volume of the metronome in front of the track faders. It scrolls with them.
            Rectangle {
                id: metronomeCard

                width: rowsColumn.width
                height: visible ? metronomeContent.height + 2 * prv.cardSpacing : 0
                visible: root.metronomeVisible

                radius: 6
                color: ui.theme.backgroundPrimaryColor
                border.width: 1
                border.color: ui.theme.strokeColor

                Column {
                    id: metronomeContent

                    anchors.top: parent.top
                    anchors.topMargin: prv.cardSpacing
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: prv.cardSpacing
                    anchors.rightMargin: prv.margins

                    spacing: 4

                    Item {
                        width: parent.width
                        height: 32

                        StyledIconLabel {
                            id: metronomeIcon

                            anchors.left: parent.left
                            anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter

                            iconCode: IconCode.METRONOME
                            font: Qt.font({ family: ui.theme.iconsFont.family, pixelSize: 24 })
                        }

                        StyledTextLabel {
                            id: metronomeTitle

                            anchors.left: metronomeIcon.right
                            anchors.leftMargin: prv.margins
                            anchors.right: metronomeValue.left
                            anchors.rightMargin: 6
                            anchors.verticalCenter: parent.verticalCenter

                            horizontalAlignment: Text.AlignLeft
                            font: ui.theme.bodyBoldFont
                            text: qsTrc("appshell", "Metronome")
                        }

                        StyledTextLabel {
                            id: metronomeValue

                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter

                            font: ui.theme.bodyBoldFont
                            text: metronomeSlider.valueText
                        }
                    }

                    HorizontalVolumeSlider {
                        id: metronomeSlider

                        width: parent.width

                        from: mixer.minMetronomeDb
                        to: mixer.maxMetronomeDb
                        defaultDb: 0
                        volumeDb: mixer.metronomeDb

                        navigation.name: "MetronomeSlider"
                        navigation.panel: metronomeNavPanel
                        navigation.order: 1
                        navigation.accessible.name: metronomeTitle.text + " " + metronomeValue.text

                        onVolumeDbMoved: function(db) {
                            mixer.metronomeDb = db
                        }
                    }
                }
            }

            Repeater {
                model: mixer

                delegate: Item {
                    id: row

                    required property int index
                    required property string rowType
                    required property string title
                    required property bool isOn
                    required property bool isSolo
                    required property real volumeDb
                    required property real effectiveDb
                    required property bool groupOn
                    required property bool groupSolo
                    required property bool isExpanded
                    required property int memberCount

                    readonly property bool isGroup: row.rowType === "group"
                    readonly property bool isMember: row.rowType === "member"
                    readonly property bool isFooter: row.rowType === "footer"

                    width: rowsColumn.width
                    height: row.isFooter ? footerButton.height : card.height

                    NavigationPanel {
                        id: rowNavPanel

                        name: "RehearsalMixerRow" + row.index
                        section: root.navigationSection
                        order: root.navigationOrderStart + 3 + row.index
                        enabled: root.enabled && root.visible
                        direction: NavigationPanel.Horizontal
                        accessible.name: row.title

                        //! Scrolls the row into view when it gets the keyboard focus
                        onActiveChanged: function(active) {
                            if (active) {
                                Utils.ensureContentVisible(flickable, Qt.rect(row.x, row.y, row.width, row.height), prv.cardSpacing)
                            }
                        }
                    }

                    //! Connects the members of an expanded group
                    Rectangle {
                        visible: row.isMember || row.isFooter

                        x: prv.memberIndent / 2 - width / 2
                        y: -prv.cardSpacing
                        width: 2
                        height: row.height + prv.cardSpacing - (row.isFooter ? row.height / 2 : 0)

                        color: Utils.colorWithAlpha(ui.theme.fontPrimaryColor, 0.25)
                    }

                    FlatButton {
                        id: footerButton

                        visible: row.isFooter

                        x: prv.memberIndent
                        height: prv.touchSize

                        icon: IconCode.DELETE_TANK
                        text: qsTrc("appshell", "Dissolve group")
                        orientation: Qt.Horizontal
                        transparent: true

                        toolTipTitle: qsTrc("appshell", "Dissolve group")
                        toolTipDescription: qsTrc("appshell", "The tracks get their own faders again")

                        navigation.name: "DissolveGroupButton"
                        navigation.panel: rowNavPanel
                        navigation.order: 1
                        navigation.enabled: row.isFooter
                        navigation.accessible.name: qsTrc("appshell", "Dissolve group") + " " + row.title

                        onClicked: {
                            mixer.dissolveGroup(row.index)
                        }
                    }

                    Rectangle {
                        id: card

                        readonly property string statusText: {
                            if (row.isGroup) {
                                //: %1 is the number of tracks in a fader group
                                const tracks = qsTrc("appshell", "%1 tracks").arg(row.memberCount)

                                if (!row.isOn) {
                                    return tracks + " · " + qsTrc("appshell", "muted")
                                }

                                if (row.isSolo) {
                                    return tracks + " · " + qsTrc("appshell", "solo")
                                }

                                return tracks
                            }

                            if (!row.isOn) {
                                return qsTrc("appshell", "muted")
                            }

                            if (row.isMember && !row.groupOn) {
                                return qsTrc("appshell", "muted by group")
                            }

                            if (row.isSolo || (row.isMember && row.groupSolo)) {
                                return qsTrc("appshell", "solo")
                            }

                            if (mixer.hasSolo) {
                                //: %1 is the effective volume relative to the score, e.g. "-6.0 dB"
                                return qsTrc("appshell", "accompaniment · effective %1").arg(prv.dbText(row.effectiveDb))
                            }

                            if (row.isMember && row.effectiveDb !== row.volumeDb) {
                                //: %1 is the effective volume relative to the score, e.g. "-6.0 dB"
                                return qsTrc("appshell", "effective %1").arg(prv.dbText(row.effectiveDb))
                            }

                            return qsTrc("appshell", "audible")
                        }

                        visible: !row.isFooter

                        x: row.isMember ? prv.memberIndent : 0
                        width: row.width - x
                        height: cardContent.height + 2 * prv.cardSpacing

                        radius: 6
                        color: row.isMember ? ui.theme.backgroundSecondaryColor : ui.theme.backgroundPrimaryColor
                        border.width: 1
                        border.color: row.isSolo ? prv.goldColor : ui.theme.strokeColor

                        //! Marks a group fader
                        Rectangle {
                            visible: row.isGroup

                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.margins: 1

                            width: 4
                            radius: 2
                            color: ui.theme.accentColor
                        }

                        Column {
                            id: cardContent

                            anchors.top: parent.top
                            anchors.topMargin: prv.cardSpacing
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: prv.cardSpacing
                            anchors.rightMargin: prv.margins

                            spacing: 4

                            Item {
                                width: parent.width
                                height: prv.touchSize

                                FlatButton {
                                    id: expandButton

                                    anchors.left: parent.left
                                    anchors.verticalCenter: parent.verticalCenter

                                    visible: row.isGroup
                                    width: visible ? 32 : 0
                                    height: prv.touchSize

                                    icon: row.isExpanded ? IconCode.SMALL_ARROW_DOWN : IconCode.SMALL_ARROW_RIGHT
                                    iconFont: Qt.font({ family: ui.theme.iconsFont.family, pixelSize: 24 })
                                    transparent: true

                                    toolTipTitle: row.isExpanded ? qsTrc("appshell", "Collapse group")
                                                                 : qsTrc("appshell", "Expand group")

                                    navigation.name: "ExpandButton"
                                    navigation.panel: rowNavPanel
                                    navigation.order: 1
                                    navigation.enabled: row.isGroup
                                    navigation.accessible.name: row.title + " " + toolTipTitle

                                    onClicked: {
                                        mixer.toggleExpanded(row.index)
                                    }
                                }

                                FlatToggleButton {
                                    id: muteButton

                                    anchors.left: expandButton.right
                                    anchors.leftMargin: row.isGroup ? 2 : 0
                                    anchors.verticalCenter: parent.verticalCenter

                                    width: prv.touchSize
                                    height: prv.touchSize

                                    icon: IconCode.MUTE
                                    checked: !row.isOn
                                    toolTipTitle: qsTrc("appshell", "Mute")

                                    navigation.name: "MuteButton"
                                    navigation.panel: rowNavPanel
                                    navigation.order: 2
                                    navigation.enabled: !row.isFooter
                                    navigation.accessible.name: row.title + " " + qsTrc("appshell", "Mute")

                                    onToggled: {
                                        mixer.setOn(row.index, !row.isOn)
                                    }
                                }

                                FlatToggleButton {
                                    id: soloButton

                                    anchors.left: muteButton.right
                                    anchors.leftMargin: 6
                                    anchors.verticalCenter: parent.verticalCenter

                                    width: prv.touchSize
                                    height: prv.touchSize

                                    icon: IconCode.SOLO
                                    checked: row.isSolo
                                    toolTipTitle: qsTrc("appshell", "Solo")

                                    navigation.name: "SoloButton"
                                    navigation.panel: rowNavPanel
                                    navigation.order: 3
                                    navigation.enabled: !row.isFooter
                                    navigation.accessible.name: row.title + " " + qsTrc("appshell", "Solo")

                                    onToggled: {
                                        mixer.setSolo(row.index, !row.isSolo)
                                    }
                                }

                                Column {
                                    id: titleColumn

                                    anchors.left: soloButton.right
                                    anchors.leftMargin: prv.margins
                                    anchors.right: volumeLabel.left
                                    anchors.rightMargin: 6
                                    anchors.verticalCenter: parent.verticalCenter

                                    spacing: 2

                                    StyledTextLabel {
                                        width: parent.width

                                        horizontalAlignment: Text.AlignLeft
                                        font: ui.theme.bodyBoldFont
                                        text: row.title
                                    }

                                    StyledTextLabel {
                                        width: parent.width

                                        horizontalAlignment: Text.AlignLeft
                                        opacity: 0.7
                                        text: card.statusText
                                    }
                                }

                                //! Tapping the name of a group expands or collapses it
                                MouseArea {
                                    anchors.fill: titleColumn
                                    enabled: row.isGroup

                                    onClicked: {
                                        mixer.toggleExpanded(row.index)
                                    }
                                }

                                StyledTextLabel {
                                    id: volumeLabel

                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter

                                    font: ui.theme.bodyBoldFont
                                    text: volumeSlider.valueText
                                }
                            }

                            HorizontalVolumeSlider {
                                id: volumeSlider

                                width: parent.width

                                enabled: row.isOn
                                from: mixer.minVolumeDb
                                to: mixer.maxVolumeDb
                                defaultDb: mixer.maxVolumeDb
                                volumeDb: row.volumeDb

                                navigation.name: "VolumeSlider"
                                navigation.panel: rowNavPanel
                                navigation.order: 4
                                navigation.enabled: !row.isFooter
                                navigation.accessible.name: row.title + " " + qsTrc("appshell", "Volume") + " " + volumeLabel.text

                                onVolumeDbMoved: function(db) {
                                    mixer.setVolumeDb(row.index, db)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
