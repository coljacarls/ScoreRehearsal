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
import MuseScore.AppShell

//! What the audience sees: only the score and the playhead. Used by the detached window of the desktop version
//! (PresentationWindow) and by the page of the presentation tab of the browser version (PresentationPage).
//! The playhead stays near the left edge and the score scrolls underneath it (karaoke style).
//! It follows the playback position only, not the zoom and position of the rehearsal view.
Item {
    id: root

    //! Parts that are solo in the rehearsal view: the rest of the score is shaded
    property var soloPartIds: []

    //! The strokes drawn with the pen of the laser pointer (lists of points in logical score coordinates)
    property var strokes: []
    property var liveStroke: []

    //! The highlighted rectangle of the laser pointer (logical score coordinates)
    property bool hasHighlight: false
    property rect highlight: Qt.rect(0, 0, 0, 0)

    //! The notes that sound with the pitch pipe or the chord scrubbing (areas in logical score coordinates),
    //! pointed at by a hand each
    property var pitchRects: []

    //! The presentation tab of the browser version has no rehearsal view that sets the continuous view of the score
    property bool forceContinuousView: false

    signal doubleClicked()

    onHighlightChanged: scoreView.setHighlight(root.highlight)

    //! The browser version: the position of the playback of the rehearsal tab
    function setRemotePlayback(secs, playing) {
        scoreView.setRemotePlayback(secs, playing)
    }

    //! The browser version: the presentation settings of the rehearsal tab (percent)
    function applySettings(shadePercent, maxStaffHeightPercent) {
        if (shadePercent !== undefined && shadePercent !== settings.shadePercent) {
            settings.shadePercent = shadePercent
        }
        if (maxStaffHeightPercent !== undefined && maxStaffHeightPercent !== settings.maxStaffHeightPercent) {
            settings.maxStaffHeightPercent = maxStaffHeightPercent
        }
    }

    Rectangle {
        anchors.fill: parent
        color: ui.theme.backgroundPrimaryColor
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
            if (root.forceContinuousView) {
                scoreView.useContinuousView()
            }
            scoreView.load()
            scoreView.setHighlight(root.highlight)
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

        anchors.fill: parent

        area: scoreView.highlightRect
        shown: root.hasHighlight
        shadeColor: prv.shadeColor
    }

    //! The strokes: on top of the shade and the frame
    LaserDrawing {
        anchors.fill: parent

        paintView: scoreView
        strokes: root.strokes
        liveStroke: root.liveStroke
    }

    PitchPipeHand {
        anchors.fill: parent

        paintView: scoreView
        logicRects: root.pitchRects
    }

    PresentationSettingsModel {
        id: settings
    }

    QtObject {
        id: prv

        readonly property color shadeColor: Qt.rgba(0, 0, 0, settings.shadePercent / 100)
    }

    MouseArea {
        anchors.fill: parent
        onDoubleClicked: root.doubleClicked()
    }
}
