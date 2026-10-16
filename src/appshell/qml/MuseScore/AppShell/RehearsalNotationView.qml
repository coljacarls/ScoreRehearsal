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
import MuseScore.NotationScene
import MuseScore.AppShell

//! The score view of the rehearsal page. With the hand (default) the score can be dragged and a tap places
//! the playhead. With the laser pointer the score is not dragged: depending on the tool a tap frames the tapped
//! notes (rectangle), a finger or the mouse draws (pen), or annotations are selected, resized and deleted (select).
//! The annotations belong to the page (they are saved in the session) and are always shown.
Item {
    id: root

    property alias name: notationView.name
    readonly property alias navigationSection: notationView.navigationSection
    property alias viewModeOverride: notationView.viewModeOverride

    //! Laser pointer instead of the hand, and its tool: 0 = rectangle, 1 = pen, 2 = select
    property bool laserActive: false
    property int laserMode: 0

    //! The annotations (logical score coordinates): strokes are lists of points, the highlight is a rectangle
    property var strokes: []
    property var liveStroke: []
    property bool hasHighlight: false
    property rect highlight: Qt.rect(0, 0, 0, 0)

    //! Rectangle tool: a tap (rect of the tapped element)
    signal laserTapped(rect logicRect)
    //! Pen: the stroke that is being drawn
    signal strokeProgress(var points)
    signal strokeFinished(var points)
    //! Select tool
    signal highlightEdited(rect logicRect)
    signal highlightDeleteRequested()
    signal strokeDeleteRequested(int index)

    //! Select tool: what is selected ("highlight", "stroke" or "")
    property string selectedKind: ""
    property int selectedStroke: -1

    //! Counts every change of the zoom or position of the score: positions in the view are calculated again then
    property int viewRevision: 0

    readonly property bool selecting: root.laserActive && root.laserMode === 2
    readonly property real hitDistance: 14
    readonly property real handleSize: 16

    function toView(x, y) {
        root.viewRevision
        const paintView = notationView.paintView
        const viewRect = paintView.logicalToView(Qt.rect(x, y, 0, 0))
        const pos = paintView.mapToItem(root, viewRect.x, viewRect.y)
        return Qt.point(pos.x, pos.y)
    }

    function areaToView(area) {
        const topLeft = root.toView(area.x, area.y)
        const bottomRight = root.toView(area.x + area.width, area.y + area.height)
        return Qt.rect(topLeft.x, topLeft.y, bottomRight.x - topLeft.x, bottomRight.y - topLeft.y)
    }

    function toLogical(x, y) {
        const paintView = notationView.paintView
        const pos = root.mapToItem(paintView, x, y)
        return paintView.viewToLogical(Qt.point(pos.x, pos.y))
    }

    function distanceToSegment(p, a, b) {
        const dx = b.x - a.x
        const dy = b.y - a.y
        const lengthSquared = dx * dx + dy * dy
        let t = lengthSquared > 0 ? ((p.x - a.x) * dx + (p.y - a.y) * dy) / lengthSquared : 0
        t = Math.max(0, Math.min(1, t))
        return Math.hypot(p.x - (a.x + t * dx), p.y - (a.y + t * dy))
    }

    //! Smallest distance of a point (view coordinates) to a stroke
    function distanceToStroke(p, stroke) {
        let best = Infinity
        let previous = null
        for (let i = 0; i < stroke.length; ++i) {
            const current = root.toView(stroke[i].x, stroke[i].y)
            best = Math.min(best, previous ? root.distanceToSegment(p, previous, current) : Math.hypot(p.x - current.x, p.y - current.y))
            previous = current
        }
        return best
    }

    function strokeBoundsInView(index) {
        const stroke = root.strokes[index]
        if (!stroke || stroke.length === 0) {
            return Qt.rect(0, 0, 0, 0)
        }

        let left = Infinity
        let top = Infinity
        let right = -Infinity
        let bottom = -Infinity
        for (const point of stroke) {
            const p = root.toView(point.x, point.y)
            left = Math.min(left, p.x)
            top = Math.min(top, p.y)
            right = Math.max(right, p.x)
            bottom = Math.max(bottom, p.y)
        }
        return Qt.rect(left, top, right - left, bottom - top)
    }

    function clearSelection() {
        root.selectedKind = ""
        root.selectedStroke = -1
    }

    onSelectingChanged: {
        if (!root.selecting) {
            root.clearSelection()
        }
    }

    onStrokesChanged: {
        if (root.selectedKind === "stroke" && root.selectedStroke >= root.strokes.length) {
            root.clearSelection()
        }
    }

    onHasHighlightChanged: {
        if (!root.hasHighlight && root.selectedKind === "highlight") {
            root.clearSelection()
        }
    }

    NotationView {
        id: notationView

        anchors.fill: parent

        readOnly: true
        tapToSeek: true
        reportTaps: root.laserActive

        onTapped: function(logicRect) {
            root.laserTapped(logicRect)
        }

        //! No score tabs: the app shows a single score and never saves it
        isSwitchPanelVisible: false
    }

    Connections {
        target: notationView.paintView

        function onMatrixChanged() { root.viewRevision += 1 }
        function onWidthChanged() { root.viewRevision += 1 }
        function onHeightChanged() { root.viewRevision += 1 }
        function onXChanged() { root.viewRevision += 1 }
        function onYChanged() { root.viewRevision += 1 }
    }

    PresentationSettingsModel {
        id: settings
    }

    //! The highlighted area with the shade around it
    LaserHighlight {
        id: laser

        anchors.fill: parent

        area: root.hasHighlight ? root.areaToView(root.highlight) : Qt.rect(0, 0, 0, 0)
        shown: root.hasHighlight
        shadeColor: Qt.rgba(0, 0, 0, settings.shadePercent / 100)
    }

    //! The strokes are on top of everything, above the shade of the highlight
    LaserDrawing {
        anchors.fill: parent

        paintView: notationView.paintView
        strokes: root.strokes
        liveStroke: root.liveStroke
        selectedIndex: root.selectedKind === "stroke" ? root.selectedStroke : -1
    }

    //! Pen: takes the mouse and touch input, drawing instead of moving the score
    MouseArea {
        id: penArea

        property var points: []

        anchors.fill: parent
        enabled: root.laserActive && root.laserMode === 1
        visible: enabled
        acceptedButtons: Qt.LeftButton
        preventStealing: true
        cursorShape: Qt.CrossCursor

        function finish() {
            if (penArea.points.length === 1) {
                //! A tap makes a dot
                const p = penArea.points[0]
                penArea.points = [p, Qt.point(p.x + 0.01, p.y)]
            }

            if (penArea.points.length > 0) {
                root.strokeFinished(penArea.points)
            }

            penArea.points = []
        }

        onPressed: function(mouse) {
            penArea.points = [root.toLogical(mouse.x, mouse.y)]
            root.strokeProgress(penArea.points)
        }

        onPositionChanged: function(mouse) {
            if (!penArea.pressed || penArea.points.length === 0) {
                return
            }

            penArea.points = penArea.points.concat([root.toLogical(mouse.x, mouse.y)])
            root.strokeProgress(penArea.points)
        }

        onReleased: penArea.finish()
        onCanceled: penArea.finish()
    }

    //! Select: selects a stroke or the highlight; the corners of the selected highlight can be dragged
    MouseArea {
        id: selectArea

        //! Corner being dragged (0 top left, 1 top right, 2 bottom right, 3 bottom left), -1: none
        property int corner: -1
        //! The opposite corner stays where it is (logical coordinates)
        property point fixedCorner: Qt.point(0, 0)

        anchors.fill: parent
        enabled: root.selecting
        visible: enabled
        acceptedButtons: Qt.LeftButton
        preventStealing: true

        function cornersInView() {
            const r = root.areaToView(root.highlight)
            return [Qt.point(r.x, r.y), Qt.point(r.x + r.width, r.y), Qt.point(r.x + r.width, r.y + r.height),
                    Qt.point(r.x, r.y + r.height)]
        }

        onPressed: function(mouse) {
            const pos = Qt.point(mouse.x, mouse.y)

            if (root.selectedKind === "highlight") {
                const corners = selectArea.cornersInView()
                for (let i = 0; i < 4; ++i) {
                    if (Math.hypot(pos.x - corners[i].x, pos.y - corners[i].y) <= root.hitDistance + root.handleSize / 2) {
                        const opposite = corners[(i + 2) % 4]
                        selectArea.corner = i
                        selectArea.fixedCorner = root.toLogical(opposite.x, opposite.y)
                        return
                    }
                }
            }

            //! The strokes are on top: the last drawn stroke first
            for (let i = root.strokes.length - 1; i >= 0; --i) {
                if (root.distanceToStroke(pos, root.strokes[i]) <= root.hitDistance) {
                    root.selectedKind = "stroke"
                    root.selectedStroke = i
                    return
                }
            }

            if (root.hasHighlight) {
                const area = root.areaToView(root.highlight)
                if (pos.x >= area.x - 6 && pos.x <= area.x + area.width + 6
                    && pos.y >= area.y - 6 && pos.y <= area.y + area.height + 6) {
                    root.selectedKind = "highlight"
                    root.selectedStroke = -1
                    return
                }
            }

            root.clearSelection()
        }

        onPositionChanged: function(mouse) {
            if (selectArea.corner < 0 || !selectArea.pressed) {
                return
            }

            const p = root.toLogical(mouse.x, mouse.y)
            const fixed = selectArea.fixedCorner
            root.highlightEdited(Qt.rect(Math.min(p.x, fixed.x), Math.min(p.y, fixed.y),
                                         Math.max(Math.abs(p.x - fixed.x), 4), Math.max(Math.abs(p.y - fixed.y), 4)))
        }

        onReleased: selectArea.corner = -1
        onCanceled: selectArea.corner = -1
    }

    //! The handles at the corners of the selected highlight
    Repeater {
        model: root.selecting && root.selectedKind === "highlight" && root.hasHighlight ? 4 : 0

        Rectangle {
            required property int index

            readonly property rect area: root.areaToView(root.highlight)

            x: (index === 0 || index === 3 ? area.x : area.x + area.width) - width / 2
            y: (index < 2 ? area.y : area.y + area.height) - height / 2
            width: root.handleSize
            height: root.handleSize
            radius: 3

            color: "white"
            border.width: 2
            border.color: "#FF8C00"
        }
    }

    //! Deletes the selected highlight / stroke
    FlatButton {
        id: deleteSelectionButton

        readonly property rect bounds: root.selectedKind === "highlight" ? root.areaToView(root.highlight)
                                                                         : root.strokeBoundsInView(root.selectedStroke)

        x: Math.max(0, Math.min(bounds.x + bounds.width + 10, root.width - width))
        y: Math.max(0, Math.min(bounds.y - height - 10, root.height - height))
        width: 36
        height: 36

        visible: root.selecting && (root.selectedKind === "highlight" || root.selectedKind === "stroke")
        icon: IconCode.DELETE_TANK
        iconFont: Qt.font({ family: ui.theme.iconsFont.family, pixelSize: 22 })

        toolTipTitle: qsTrc("appshell", "Delete")

        onClicked: {
            if (root.selectedKind === "highlight") {
                root.highlightDeleteRequested()
            } else {
                root.strokeDeleteRequested(root.selectedStroke)
            }

            root.clearSelection()
        }
    }
}
