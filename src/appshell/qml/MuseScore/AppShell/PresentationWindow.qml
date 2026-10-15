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
import QtQuick.Window

import Muse.Ui
import Muse.UiComponents
import MuseScore.AppShell

//! Detached window for the audience: only the score and the playhead.
//! The playhead stays near the left edge and the score scrolls underneath it (karaoke style).
//! It follows the playback position only, not the zoom and position of the rehearsal view.
Window {
    id: root

    //! Remembered by the rehearsal page between openings
    property rect initialGeometry: Qt.rect(0, 0, 1280, 360)

    //! Parts that are solo in the rehearsal view: the rest of the score is shaded
    property var soloPartIds: []
    signal closeRequested(rect geometry)

    title: qsTrc("appshell", "Presentation")
    color: ui.theme.backgroundPrimaryColor

    x: initialGeometry.x
    y: initialGeometry.y
    width: initialGeometry.width
    height: initialGeometry.height
    minimumWidth: 320
    minimumHeight: 120

    visible: true

    //! Laser pointer: frames an area of the score (logical coordinates) until clearHighlight()
    function highlight(logicRect) {
        scoreView.setHighlight(logicRect)
        laser.shown = true
    }

    function clearHighlight() {
        laser.shown = false
    }

    function toggleFullScreen() {
        root.visibility = (root.visibility === Window.FullScreen) ? Window.Windowed : Window.FullScreen
    }

    onClosing: {
        //! The geometry of the normal state, not of the fullscreen one
        root.closeRequested(root.visibility === Window.FullScreen ? root.initialGeometry
                                                                  : Qt.rect(root.x, root.y, root.width, root.height))
    }

    PresentationPaintView {
        id: scoreView

        anchors.fill: parent

        //! Read-only: nothing here reacts to mouse or touch
        enabled: false

        //! The playhead is drawn by the view itself at a fixed place (not by the base class cursor)
        Rectangle {
            x: scoreView.playheadRect.x
            y: scoreView.playheadRect.y
            width: scoreView.playheadRect.width
            height: scoreView.playheadRect.height

            visible: scoreView.playheadRect.width > 0
            color: scoreView.playheadColor
        }

        soloPartIds: root.soloPartIds

        Component.onCompleted: {
            scoreView.load()
        }
    }

    //! Fixed above the score: only the score underneath moves while playing.
    //! Dark stripes over everything but the solo staves, with a soft gradient (not a hard edge) to them.
    Repeater {
        model: scoreView.shadeBands

        Rectangle {
            required property var modelData

            x: 0
            y: modelData.y
            width: root.width
            height: modelData.height

            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: modelData.fadeTop > 0 ? "transparent" : prv.shadeColor
                }
                GradientStop {
                    position: modelData.fadeTop / modelData.height
                    color: prv.shadeColor
                }
                GradientStop {
                    position: 1.0 - modelData.fadeBottom / modelData.height
                    color: prv.shadeColor
                }
                GradientStop {
                    position: 1.0
                    color: modelData.fadeBottom > 0 ? "transparent" : prv.shadeColor
                }
            }
        }
    }

    LaserHighlight {
        id: laser

        area: scoreView.highlightRect
    }

    QtObject {
        id: prv

        readonly property color shadeColor: Qt.rgba(0, 0, 0, 0.72)
    }

    MouseArea {
        anchors.fill: parent
        onDoubleClicked: root.toggleFullScreen()
    }

    Shortcut {
        sequence: "F11"
        context: Qt.WindowShortcut
        onActivated: root.toggleFullScreen()
    }

    Shortcut {
        sequence: "Esc"
        context: Qt.WindowShortcut
        enabled: root.visibility === Window.FullScreen
        onActivated: root.visibility = Window.Windowed
    }
}
