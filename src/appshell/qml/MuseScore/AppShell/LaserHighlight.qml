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

//! The laser pointer highlight: a solid orange frame around an area of the score (view coordinates).
//! It stays as long as `shown` is true and fades in and out.
Item {
    id: root

    //! The area to highlight; the line lies on its border
    property rect area: Qt.rect(0, 0, 0, 0)
    property bool shown: false

    readonly property color frameColor: "#FF8C00"
    readonly property real lineWidth: 3
    readonly property int fadeDuration: 250
    readonly property int blinkInterval: 300
    readonly property int blinkCount: 3

    //! A new frame flashes a few times before it stays: blinkStep counts the switches (on, off, on, off, ..., on)
    property bool blinking: false
    property int blinkStep: 0

    onShownChanged: {
        if (root.shown) {
            root.blinkStep = 0
            root.blinking = true
            blinkTimer.restart()
        } else {
            root.blinking = false
            blinkTimer.stop()
        }
    }

    x: area.x - lineWidth / 2
    y: area.y - lineWidth / 2
    width: area.width + lineWidth
    height: area.height + lineWidth

    opacity: !root.shown ? 0 : (root.blinking && root.blinkStep % 2 === 1) ? 0 : 1
    visible: opacity > 0
    enabled: false

    Behavior on opacity {
        enabled: !root.blinking

        NumberAnimation {
            duration: root.fadeDuration
        }
    }

    Timer {
        id: blinkTimer

        interval: root.blinkInterval
        repeat: true

        onTriggered: {
            root.blinkStep += 1
            if (root.blinkStep >= 2 * root.blinkCount) {
                root.blinking = false
                blinkTimer.stop()
            }
        }
    }

    Rectangle {
        anchors.fill: parent

        color: "transparent"
        border.width: root.lineWidth
        border.color: root.frameColor
    }
}
