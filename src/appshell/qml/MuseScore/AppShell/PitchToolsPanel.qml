/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited and others
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

import QtQuick

import Muse.Ui
import Muse.UiComponents
import MuseScore.AppShell

//! The floating panel with the tools to give pitches (bottom right over the score): the pitch pipe (a touched note
//! sounds as long as it is held, the notes the finger passes sound in turn) and the chord scrubbing (the chord at
//! the finger sounds, without moving the playhead). With both the score is not dragged.
Rectangle {
    id: root

    required property RehearsalPageModel pageModel

    property NavigationSection navigationSection: null
    property int navigationOrderStart: 1

    readonly property int buttonSize: 40

    implicitWidth: row.implicitWidth + 12 * 2
    implicitHeight: root.buttonSize + 12
    width: implicitWidth
    height: implicitHeight
    radius: height / 2

    color: Utils.colorWithAlpha(ui.theme.backgroundPrimaryColor, 0.9)
    border.color: ui.theme.strokeColor
    border.width: ui.theme.borderWidth

    //! Do not let clicks on the panel reach the score
    MouseArea {
        anchors.fill: parent
    }

    NavigationPanel {
        id: navPanel
        name: "RehearsalPitchTools"
        enabled: root.enabled && root.visible
        section: root.navigationSection
        order: root.navigationOrderStart
        direction: NavigationPanel.Horizontal
        accessible.name: qsTrc("appshell", "Pitch tools")
    }

    Row {
        id: row

        anchors.centerIn: parent
        spacing: 4

        RehearsalToolButton {
            width: root.buttonSize
            height: root.buttonSize
            iconSize: 26

            kind: "tuningfork"
            gold: false
            selected: root.pageModel.isPitchPipeActive

            toolTipTitle: qsTrc("appshell", "Pitch pipe")
            toolTipDescription: qsTrc("appshell", "Touch a note to hear it as long as you hold it, slide over the notes to hear each of them")

            navigation.panel: navPanel
            navigation.order: 1

            onClicked: {
                root.pageModel.isPitchPipeActive = !root.pageModel.isPitchPipeActive
            }
        }

        RehearsalToolButton {
            width: root.buttonSize
            height: root.buttonSize
            iconSize: 26

            kind: "chordscrub"
            gold: false
            selected: root.pageModel.isChordScrubActive

            toolTipTitle: qsTrc("appshell", "Chord scrubbing")
            toolTipDescription: qsTrc("appshell", "Hold the finger on the score and move it to hear the chord at each position, the playhead stays where it is")

            navigation.panel: navPanel
            navigation.order: 2

            onClicked: {
                root.pageModel.isChordScrubActive = !root.pageModel.isChordScrubActive
            }
        }
    }
}
