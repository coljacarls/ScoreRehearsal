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

import QtQuick

import Muse.Ui
import Muse.UiComponents

//! Shown while no score is open
Item {
    id: root

    signal openRequested()

    NavigationPanel {
        id: navPanel
        name: "OpenScorePlaceholder"
        enabled: root.enabled && root.visible
        order: 3
        direction: NavigationPanel.Vertical
        accessible.name: qsTrc("appshell", "Open score")
    }

    Column {
        anchors.centerIn: parent
        width: Math.min(420, parent.width - 48)

        spacing: 16

        Image {
            anchors.horizontalCenter: parent.horizontalCenter

            source: "resources/scorerehearsal_logo.png"
            sourceSize: Qt.size(96, 96)
            width: 96
            height: 96
        }

        StyledTextLabel {
            width: parent.width
            text: "ScoreRehearsal"
            font: ui.theme.headerBoldFont
            horizontalAlignment: Text.AlignHCenter
        }

        StyledTextLabel {
            width: parent.width
            text: qsTrc("appshell", "Open a MuseScore file (.mscz, .mscx) to rehearse with it, or drop it onto this window. The file is never changed.")
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        FlatButton {
            anchors.horizontalCenter: parent.horizontalCenter
            height: 40

            icon: IconCode.OPEN_FILE
            text: qsTrc("appshell", "Open score…")
            orientation: Qt.Horizontal
            accentButton: true

            navigation.panel: navPanel
            navigation.order: 1

            onClicked: {
                root.openRequested()
            }
        }
    }
}
