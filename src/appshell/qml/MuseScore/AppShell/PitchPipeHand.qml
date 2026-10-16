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

//! Points at the notes that sound (pitch pipe, chord scrubbing): a hand with the index finger down above each note.
//! The notes are given in logical score coordinates; the zoom and position of the paint view that shows the score
//! (placed under this item) are read from it, so the hands stay on the notes in every view.
Item {
    id: root

    //! The paint view that shows the score (placed under this item)
    required property var paintView

    //! The areas of the notes in logical score coordinates: [rect, ...]; empty: no hand
    property var logicRects: []

    //! Counts the changes of the zoom and position of the view: the hands are placed again then
    property int revision: 0

    function toView(logicRect, revision) {
        const view = root.paintView
        const rect = view.logicalToView(logicRect)
        const topLeft = view.mapToItem(root, rect.x, rect.y)
        const bottomRight = view.mapToItem(root, rect.x + rect.width, rect.y + rect.height)
        return Qt.rect(topLeft.x, topLeft.y, bottomRight.x - topLeft.x, bottomRight.y - topLeft.y)
    }

    enabled: false
    clip: true

    Connections {
        target: root.paintView

        function onMatrixChanged() { root.revision += 1 }
        function onWidthChanged() { root.revision += 1 }
        function onHeightChanged() { root.revision += 1 }
        function onXChanged() { root.revision += 1 }
        function onYChanged() { root.revision += 1 }
    }

    //! The hands bob together
    property real bob: 0

    SequentialAnimation on bob {
        running: root.logicRects.length > 0
        loops: Animation.Infinite

        NumberAnimation {
            from: 0
            to: 6
            duration: 450
            easing.type: Easing.InOutSine
        }

        NumberAnimation {
            from: 6
            to: 0
            duration: 450
            easing.type: Easing.InOutSine
        }
    }

    Repeater {
        model: root.logicRects

        Text {
            id: hand

            required property var modelData

            //! In this item's coordinates; the revision makes the binding follow the view
            readonly property rect area: root.toView(hand.modelData, root.revision)

            //! The tip of the finger is at the bottom of the glyph: it is lowered a little over the note
            readonly property real lift: 0.9

            visible: hand.modelData.width > 0 && hand.modelData.height > 0

            text: "\uD83D\uDC47"
            font.family: Qt.platform.os === "windows" ? "Segoe UI Emoji"
                                                     : (Qt.platform.os === "osx" ? "Apple Color Emoji" : "Noto Color Emoji")
            //! About three staff spaces, so that the hand is neither lost at a small zoom nor huge at a big one
            font.pixelSize: Math.max(28, Math.min(90, hand.area.height * 3.2))

            x: hand.area.x + hand.area.width / 2 - width / 2
            y: hand.area.y - height * hand.lift - root.bob
        }
    }
}
