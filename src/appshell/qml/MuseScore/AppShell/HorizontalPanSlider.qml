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

//! Horizontal pan slider in the look of the StyledSlider of the Muse library (colours, rounded line, round handle),
//! enlarged for touch and filled from the middle. Deliberately different from the ruler look of the volume faders,
//! so that the mode of the sidebar can be recognised at a glance. -100 = left, 0 = middle, 100 = right.
Item {
    id: root

    property int pan: 0
    property int defaultPan: 0 // restored on double click
    property int keyStep: 5

    property color accentColor: ui.theme.accentColor

    readonly property string valueText: {
        if (root.pan === 0) {
            //: Pan in the middle of the stereo image
            return qsTrc("appshell", "Center")
        }

        //: Pan to the left, %1 is a percentage
        return root.pan < 0 ? qsTrc("appshell", "L %1").arg(-root.pan) : qsTrc("appshell", "R %1").arg(root.pan)
    }

    property alias navigation: navCtrl

    signal panMoved(int pan)

    implicitHeight: prv.handleSize + prv.labelsHeight
    implicitWidth: 240

    opacity: root.enabled ? 1.0 : ui.theme.itemOpacityDisabled

    QtObject {
        id: prv

        readonly property real handleSize: 28
        readonly property real trackHeight: 4
        readonly property real labelsHeight: 14

        // Near the middle the handle snaps to it, so that the middle is easy to hit
        readonly property int snapRange: 4

        readonly property real trackStart: prv.handleSize / 2
        readonly property real trackLength: Math.max(root.width - prv.handleSize, 1)
        readonly property real trackY: prv.handleSize / 2 - prv.trackHeight / 2

        readonly property real centerX: prv.trackStart + prv.trackLength / 2
        readonly property real handleCenterX: prv.panToX(root.pan)

        property real dragStartOffset: 0.0

        function panToX(value) {
            return prv.trackStart + (value + 100) / 200 * prv.trackLength
        }

        function xToPan(x) {
            return Math.round((x - prv.trackStart) / prv.trackLength * 200 - 100)
        }

        function moveTo(value, snap) {
            var result = Math.max(-100, Math.min(Math.round(value), 100))
            if (snap && Math.abs(result) <= prv.snapRange) {
                result = 0
            }

            if (result !== root.pan) {
                root.panMoved(result)
            }
        }
    }

    NavigationControl {
        id: navCtrl
        name: root.objectName != "" ? root.objectName : "HorizontalPanSlider"
        enabled: root.enabled && root.visible

        accessible.role: MUAccessible.Range
        accessible.visualItem: root

        accessible.value: root.pan
        accessible.minimumValue: -100
        accessible.maximumValue: 100
        accessible.stepSize: root.keyStep

        onNavigationEvent: function(event) {
            switch(event.type) {
            case NavigationEvent.Left:
                prv.moveTo(root.pan - root.keyStep, false)
                event.accepted = true
                break
            case NavigationEvent.Right:
                prv.moveTo(root.pan + root.keyStep, false)
                event.accepted = true
                break
            }
        }
    }

    NavigationFocusBorder {
        navigationCtrl: navCtrl
    }

    // Line, like the blank part of the StyledSlider
    Rectangle {
        x: prv.trackStart
        y: prv.trackY
        width: prv.trackLength
        height: prv.trackHeight
        radius: prv.trackHeight

        opacity: 0.5
        color: ui.theme.fontPrimaryColor
    }

    // Marks the middle
    Rectangle {
        x: prv.centerX - width / 2
        y: prv.trackY - 4
        width: 2
        height: prv.trackHeight + 8
        radius: 1

        opacity: 0.5
        color: ui.theme.fontPrimaryColor
    }

    // From the middle to the handle, in the accent colour
    Rectangle {
        x: Math.min(prv.centerX, prv.handleCenterX)
        y: prv.trackY
        width: Math.abs(prv.handleCenterX - prv.centerX)
        height: prv.trackHeight
        radius: prv.trackHeight

        visible: root.pan !== 0
        color: root.accentColor
    }

    // Ends of the stereo image
    StyledTextLabel {
        x: prv.trackStart - prv.handleSize / 2
        y: prv.handleSize
        width: prv.handleSize
        height: prv.labelsHeight

        font: ui.theme.bodyFont
        opacity: 0.7
        //: Left channel, short
        text: qsTrc("appshell", "L")
    }

    StyledTextLabel {
        x: root.width - prv.handleSize
        y: prv.handleSize
        width: prv.handleSize
        height: prv.labelsHeight

        font: ui.theme.bodyFont
        opacity: 0.7
        //: Right channel, short
        text: qsTrc("appshell", "R")
    }

    //! Like the volume faders, the pan changes only by a horizontal swipe or by a tap on the track. A drag that is
    //! mostly vertical belongs to the Flickable of the track list.
    MouseArea {
        id: mouseArea

        property point pressPoint: Qt.point(0, 0)
        property bool swiping: false
        property bool scrolling: false

        anchors.fill: parent
        hoverEnabled: root.enabled

        preventStealing: mouseArea.swiping

        onPressed: function(mouse) {
            navCtrl.requestActiveByInteraction()

            mouseArea.pressPoint = Qt.point(mouse.x, mouse.y)
            mouseArea.swiping = false
            mouseArea.scrolling = false

            var onHandle = Math.abs(mouse.x - prv.handleCenterX) <= prv.handleSize / 2
            prv.dragStartOffset = onHandle ? mouse.x - prv.handleCenterX : 0
        }

        onPositionChanged: function(mouse) {
            if (!pressed || mouseArea.scrolling) {
                return
            }

            if (!mouseArea.swiping) {
                var dx = Math.abs(mouse.x - mouseArea.pressPoint.x)
                var dy = Math.abs(mouse.y - mouseArea.pressPoint.y)

                if (Math.max(dx, dy) < Qt.styleHints.startDragDistance) {
                    return
                }

                if (dy >= dx) {
                    mouseArea.scrolling = true
                    return
                }

                mouseArea.swiping = true
            }

            prv.moveTo(prv.xToPan(mouse.x - prv.dragStartOffset), true)
        }

        onClicked: function(mouse) {
            if (!mouseArea.swiping && !mouseArea.scrolling) {
                prv.moveTo(prv.xToPan(mouseArea.pressPoint.x), true)
            }
        }

        onCanceled: {
            mouseArea.swiping = false
            mouseArea.scrolling = false
        }

        onDoubleClicked: {
            prv.moveTo(root.defaultPan, false)
        }
    }

    // Handle
    Rectangle {
        x: prv.handleCenterX - width / 2
        y: 0
        width: prv.handleSize
        height: prv.handleSize
        radius: width / 2

        color: ui.theme.textFieldColor

        // The border of the handle of the StyledSlider: brighter while it is pressed
        Rectangle {
            anchors.fill: parent

            radius: width / 2
            opacity: mouseArea.pressed ? 1 : (mouseArea.containsMouse ? 0.5 : 0.7)
            color: "transparent"
            border.width: 1
            border.color: ui.theme.fontPrimaryColor
        }
    }
}
