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
import QtQuick.Window

import Muse.Ui
import Muse.UiComponents
//! Floating button over the score (top right): enters or leaves the immersive full screen mode
Rectangle {
    id: root

    property bool immersive: false
    signal toggleImmersiveRequested()

    property NavigationSection navigationSection: null
    property int navigationOrderStart: 1

    readonly property int buttonSize: 40

    implicitWidth: root.buttonSize + 12
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
        name: "RehearsalFullScreen"
        enabled: root.enabled && root.visible
        section: root.navigationSection
        order: root.navigationOrderStart
        direction: NavigationPanel.Horizontal
        accessible.name: qsTrc("appshell", "Full screen")
    }

    RehearsalToolButton {
        anchors.centerIn: parent
        width: root.buttonSize
        height: root.buttonSize

        kind: root.immersive ? "collapse" : "expand"
        gold: false
        selected: false

        toolTipTitle: root.immersive ? qsTrc("appshell", "Exit full screen") : qsTrc("appshell", "Full screen")
        accessible.checked: root.immersive

        navigation.panel: navPanel
        navigation.order: 1

        onClicked: {
            root.toggleImmersiveRequested()
        }
    }
}
