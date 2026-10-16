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

Item {
    id: root

    property alias name: notationView.name
    readonly property alias navigationSection: notationView.navigationSection
    property alias viewModeOverride: notationView.viewModeOverride

    //! Laser pointer mode: a tap does not seek but is reported (the page decides what to frame)
    property bool laserActive: false
    signal laserTapped(rect logicRect)

    //! The pen: the drawn strokes (lists of points in logical score coordinates) belong to the page,
    //! the view reports the stroke that is being drawn (finished: the finger or mouse was lifted)
    property bool penActive: false
    property var strokes: []
    property var liveStroke: []
    signal strokeChanged(var points, bool finished)
    signal eraseRequested()

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

    //! The highlighted area (logical score coordinates): follows the score when it is moved or zoomed
    property rect logicArea: Qt.rect(0, 0, 0, 0)
    property rect viewArea: Qt.rect(0, 0, 0, 0)

    //! Frames the area until clearHighlight()
    function highlight(logicRect) {
        root.logicArea = logicRect
        root.updateViewArea()
        laser.shown = true
    }

    function clearHighlight() {
        laser.shown = false
    }

    function updateViewArea() {
        const paintView = notationView.paintView
        const rect = paintView.logicalToView(root.logicArea)
        const topLeft = paintView.mapToItem(root, rect.x, rect.y)
        root.viewArea = Qt.rect(topLeft.x, topLeft.y, rect.width, rect.height)
    }

    Connections {
        target: notationView.paintView

        function onMatrixChanged() {
            if (laser.shown) {
                root.updateViewArea()
            }
        }
    }

    PresentationSettingsModel {
        id: settings
    }

    LaserHighlight {
        id: laser

        anchors.fill: parent

        area: root.viewArea
        shadeColor: Qt.rgba(0, 0, 0, settings.shadePercent / 100)
    }

    //! The strokes are on top of everything, above the shade of the highlight
    LaserDrawing {
        anchors.fill: parent

        paintView: notationView.paintView
        strokes: root.strokes
        liveStroke: root.liveStroke
    }

    //! Takes the mouse and touch input while the pen is active: drawing instead of moving the score
    MouseArea {
        id: penArea

        property var points: []

        anchors.fill: parent
        enabled: root.penActive
        visible: root.penActive
        acceptedButtons: Qt.LeftButton
        preventStealing: true
        cursorShape: Qt.CrossCursor

        function logicalPoint(x, y) {
            const viewPos = root.mapToItem(notationView.paintView, x, y)
            return notationView.paintView.viewToLogical(Qt.point(viewPos.x, viewPos.y))
        }

        function finish() {
            if (penArea.points.length === 1) {
                //! A tap makes a dot
                const p = penArea.points[0]
                penArea.points = [p, Qt.point(p.x + 0.01, p.y)]
            }

            if (penArea.points.length > 0) {
                root.strokeChanged(penArea.points, true)
            }

            penArea.points = []
        }

        onPressed: function(mouse) {
            penArea.points = [penArea.logicalPoint(mouse.x, mouse.y)]
            root.strokeChanged(penArea.points, false)
        }

        onPositionChanged: function(mouse) {
            if (!penArea.pressed || penArea.points.length === 0) {
                return
            }

            penArea.points = penArea.points.concat([penArea.logicalPoint(mouse.x, mouse.y)])
            root.strokeChanged(penArea.points, false)
        }

        onReleased: penArea.finish()
        onCanceled: penArea.finish()
    }

    //! Floating button over the score: erases all strokes
    FlatButton {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 16

        width: 48
        height: 48

        visible: root.strokes.length > 0 || root.liveStroke.length > 0
        icon: IconCode.ERASER
        iconFont: Qt.font({ family: ui.theme.iconsFont.family, pixelSize: 26 })

        toolTipTitle: qsTrc("appshell", "Erase all strokes")

        onClicked: {
            root.eraseRequested()
        }
    }
}
