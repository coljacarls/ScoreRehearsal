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

//! Detached window for the audience: only the score and the playhead (see PresentationView).
Window {
    id: root

    //! Remembered by the rehearsal page between openings
    property rect initialGeometry: Qt.rect(0, 0, 1280, 360)

    //! Parts that are solo in the rehearsal view: the rest of the score is shaded
    property alias soloPartIds: view.soloPartIds

    //! The strokes drawn with the pen of the laser pointer (lists of points in logical score coordinates)
    property alias strokes: view.strokes
    property alias liveStroke: view.liveStroke

    //! The highlighted rectangle of the laser pointer (logical score coordinates)
    property alias hasHighlight: view.hasHighlight
    property alias highlight: view.highlight

    //! The notes that sound with the pitch pipe or the chord scrubbing (areas in logical score coordinates),
    //! pointed at by a hand each
    property alias pitchRects: view.pitchRects

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

    function toggleFullScreen() {
        root.visibility = (root.visibility === Window.FullScreen) ? Window.Windowed : Window.FullScreen
    }

    onClosing: {
        //! The geometry of the normal state, not of the fullscreen one
        root.closeRequested(root.visibility === Window.FullScreen ? root.initialGeometry
                                                                  : Qt.rect(root.x, root.y, root.width, root.height))
    }

    PresentationView {
        id: view

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
