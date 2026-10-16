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

//! The laser pointer highlight: a solid orange frame around an area of the score, and the rest of the
//! item (the view it is placed in, fill the parent) is dimmed like the unlit staves of the solo spotlight.
//! The area is in the coordinates of this item. It stays as long as `shown` is true and fades in and out.
Item {
    id: root

    //! The area to highlight; the line lies on its border
    property rect area: Qt.rect(0, 0, 0, 0)
    property bool shown: false

    readonly property color frameColor: "#FF8C00"
    readonly property real lineWidth: 3
    readonly property int fadeDuration: 250

    //! The shade of the solo spotlight
    property color shadeColor: Qt.rgba(0, 0, 0, 0.45)

    opacity: root.shown ? 1 : 0
    visible: opacity > 0
    enabled: false

    Behavior on opacity {
        NumberAnimation {
            duration: root.fadeDuration
        }
    }

    //! The shade: four parts around the area, so that it stays clear inside the frame (no gradient)
    Rectangle {
        x: 0
        y: 0
        width: root.width
        height: Math.max(root.area.y, 0)
        color: root.shadeColor
    }

    Rectangle {
        x: 0
        y: root.area.y + root.area.height
        width: root.width
        height: Math.max(root.height - y, 0)
        color: root.shadeColor
    }

    Rectangle {
        x: 0
        y: root.area.y
        width: Math.max(root.area.x, 0)
        height: root.area.height
        color: root.shadeColor
    }

    Rectangle {
        x: root.area.x + root.area.width
        y: root.area.y
        width: Math.max(root.width - x, 0)
        height: root.area.height
        color: root.shadeColor
    }

    //! The frame, above the shade
    Rectangle {
        x: root.area.x - root.lineWidth / 2
        y: root.area.y - root.lineWidth / 2
        width: root.area.width + root.lineWidth
        height: root.area.height + root.lineWidth

        color: "transparent"
        border.width: root.lineWidth
        border.color: root.frameColor
    }
}
