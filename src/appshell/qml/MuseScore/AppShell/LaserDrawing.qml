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
import QtQuick.Shapes

//! The strokes drawn with the pen of the laser pointer, on top of everything else.
//! The strokes are lists of points in logical score coordinates, so they stay on the score in every view
//! (the zoom and position of the view are read from the paint view). The width of the line is constant on screen.
Item {
    id: root

    //! The paint view that shows the score (placed under this item)
    required property var paintView

    //! The finished strokes: [[point, ...], ...]
    property var strokes: []
    //! The stroke that is being drawn
    property var liveStroke: []

    readonly property color strokeColor: "#E5322D"
    readonly property real lineWidth: 4

    property point offset: Qt.point(0, 0)
    property real zoom: 1

    function refresh() {
        const view = root.paintView
        if (!view) {
            return
        }

        const origin = view.logicalToView(Qt.rect(0, 0, 0, 0))
        const unit = view.logicalToView(Qt.rect(1, 0, 0, 0))
        const topLeft = view.mapToItem(root, origin.x, origin.y)

        root.offset = Qt.point(topLeft.x, topLeft.y)
        root.zoom = Math.max(unit.x - origin.x, 0.0001)
    }

    enabled: false
    clip: true

    Component.onCompleted: root.refresh()

    Connections {
        target: root.paintView

        function onMatrixChanged() { root.refresh() }
        function onWidthChanged() { root.refresh() }
        function onHeightChanged() { root.refresh() }
        function onXChanged() { root.refresh() }
        function onYChanged() { root.refresh() }
    }

    onWidthChanged: root.refresh()
    onHeightChanged: root.refresh()

    Item {
        x: root.offset.x
        y: root.offset.y
        scale: root.zoom
        transformOrigin: Item.TopLeft

        Repeater {
            model: root.strokes

            Shape {
                required property var modelData

                ShapePath {
                    strokeColor: root.strokeColor
                    strokeWidth: root.lineWidth / root.zoom
                    fillColor: "transparent"
                    capStyle: ShapePath.RoundCap
                    joinStyle: ShapePath.RoundJoin

                    PathPolyline {
                        path: modelData
                    }
                }
            }
        }

        Shape {
            ShapePath {
                strokeColor: root.strokeColor
                strokeWidth: root.lineWidth / root.zoom
                fillColor: "transparent"
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin

                PathPolyline {
                    path: root.liveStroke
                }
            }
        }
    }
}
