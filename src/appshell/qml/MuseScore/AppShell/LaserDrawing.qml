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
    //! The selected stroke (index in strokes) gets a halo, -1: none
    property int selectedIndex: -1

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

    //! Rounds the corners of the polyline (Chaikin): every segment is replaced by points at 1/4 and 3/4,
    //! the ends stay where they are
    function smooth(points) {
        let result = points
        for (let pass = 0; pass < 3 && result.length > 2; ++pass) {
            const next = [result[0]]
            for (let i = 0; i < result.length - 1; ++i) {
                const a = result[i]
                const b = result[i + 1]
                next.push(Qt.point(0.75 * a.x + 0.25 * b.x, 0.75 * a.y + 0.25 * b.y))
                next.push(Qt.point(0.25 * a.x + 0.75 * b.x, 0.25 * a.y + 0.75 * b.y))
            }
            next.push(result[result.length - 1])
            result = next
        }
        return result
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
                preferredRendererType: Shape.CurveRenderer
                required property var modelData
                required property int index

                ShapePath {
                    strokeColor: root.selectedIndex === index ? Qt.rgba(1, 0.55, 0, 0.45) : "transparent"
                    strokeWidth: (root.lineWidth + 10) / root.zoom
                    fillColor: "transparent"
                    capStyle: ShapePath.RoundCap
                    joinStyle: ShapePath.RoundJoin

                    PathPolyline {
                        path: root.smooth(modelData)
                    }
                }

                ShapePath {
                    strokeColor: root.strokeColor
                    strokeWidth: root.lineWidth / root.zoom
                    fillColor: "transparent"
                    capStyle: ShapePath.RoundCap
                    joinStyle: ShapePath.RoundJoin

                    PathPolyline {
                        path: root.smooth(modelData)
                    }
                }
            }
        }

        Shape {
            preferredRendererType: Shape.CurveRenderer
            ShapePath {
                strokeColor: root.strokeColor
                strokeWidth: root.lineWidth / root.zoom
                fillColor: "transparent"
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin

                PathPolyline {
                    path: root.smooth(root.liveStroke)
                }
            }
        }
    }
}
