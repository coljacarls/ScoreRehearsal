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

//! The laser pointer highlight: a frame with a golden glow around an area of the score
//! (view coordinates). Shown for a second by flash(), then it fades out for a second.
Item {
    id: root

    //! The area to highlight; the frame is drawn around it
    property rect area: Qt.rect(0, 0, 0, 0)

    //! The same gold as the solo slider and frames in the tracks panel
    readonly property color goldColor: "#E0B23A"

    readonly property int holdDuration: 1000
    readonly property int fadeDuration: 1000

    readonly property real padding: 8
    readonly property int glowLayers: 5

    function flash() {
        animation.restart()
    }

    x: area.x - padding
    y: area.y - padding
    width: area.width + 2 * padding
    height: area.height + 2 * padding

    opacity: 0
    visible: opacity > 0
    enabled: false

    SequentialAnimation {
        id: animation

        PropertyAction {
            target: root
            property: "opacity"
            value: 1
        }

        PauseAnimation {
            duration: root.holdDuration
        }

        NumberAnimation {
            target: root
            property: "opacity"
            to: 0
            duration: root.fadeDuration
            easing.type: Easing.InQuad
        }
    }

    //! The glow: rings growing outwards that become more and more transparent
    Repeater {
        model: root.glowLayers

        Rectangle {
            required property int index

            readonly property real grow: (index + 1) * 3

            x: -grow
            y: -grow
            width: root.width + 2 * grow
            height: root.height + 2 * grow
            radius: 6 + grow

            color: "transparent"
            border.width: 3
            border.color: Qt.rgba(root.goldColor.r, root.goldColor.g, root.goldColor.b, 0.32 / (index + 1))
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: 6

        color: Qt.rgba(root.goldColor.r, root.goldColor.g, root.goldColor.b, 0.14)
        border.width: 3
        border.color: root.goldColor
    }
}
