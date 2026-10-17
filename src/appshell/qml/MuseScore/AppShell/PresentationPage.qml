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

//! The page of the presentation tab of the browser version (?mode=presentation): the whole window shows what the
//! audience sees (PresentationView). The tab plays nothing; the rehearsal tab sends the state (position of the
//! playback, solo parts, laser pointer, ...) as JSON, see RehearsalPage.qml (publishPresentationState).
Item {
    id: root

    property RehearsalPageModel pageModel: RehearsalPageModel {}

    Component.onCompleted: {
        root.pageModel.load()
    }

    //! The last state of the rehearsal tab; it may arrive before the score is open
    QtObject {
        id: prv

        property var state: null

        //! What was passed to the view the last time (as JSON): unchanged values are not set again,
        //! as the state arrives ten times per second while playing
        property var passed: ({})

        function changed(key, value) {
            const json = JSON.stringify(value === undefined ? null : value)
            if (passed[key] === json) {
                return false
            }
            passed[key] = json
            return true
        }

        function apply(view, s) {
            if (!view || !s) {
                return
            }

            const names = root.pageModel.partNames
            const part = s.part ? names.indexOf(s.part) : -1
            if (root.pageModel.currentPartIndex !== part) {
                root.pageModel.selectPart(part)
            }

            if (changed("solo", s.solo || [])) {
                view.soloPartIds = s.solo || []
            }
            if (changed("strokes", s.strokes || [])) {
                view.strokes = (s.strokes || []).map(function(stroke) {
                    return stroke.map(function(p) { return Qt.point(p.x, p.y) })
                })
            }
            if (changed("liveStroke", s.liveStroke || [])) {
                view.liveStroke = (s.liveStroke || []).map(function(p) { return Qt.point(p.x, p.y) })
            }
            if (changed("highlight", s.highlight)) {
                view.hasHighlight = !!s.hasHighlight
                if (s.highlight) {
                    view.highlight = Qt.rect(s.highlight.x, s.highlight.y, s.highlight.width, s.highlight.height)
                }
            }
            if (changed("hasHighlight", !!s.hasHighlight)) {
                view.hasHighlight = !!s.hasHighlight
            }
            if (changed("pitchRects", s.pitchRects || [])) {
                view.pitchRects = (s.pitchRects || []).map(function(r) { return Qt.rect(r.x, r.y, r.width, r.height) })
            }
            if (changed("settings", [s.shade, s.maxStaff])) {
                view.applySettings(s.shade, s.maxStaff)
            }

            view.setRemotePlayback(s.secs || 0, !!s.playing)
        }
    }

    WebBridge {
        id: web

        onStateReceived: function(json) {
            try {
                prv.state = JSON.parse(json)
            } catch (e) {
                console.warn("presentation: bad state", e)
                return
            }
            prv.apply(presentation.item, prv.state)
        }
    }

    Rectangle {
        anchors.fill: parent
        color: ui.theme.backgroundPrimaryColor
    }

    StyledTextLabel {
        anchors.centerIn: parent
        visible: !root.pageModel.hasProject
        text: qsTrc("appshell", "Opening the score…")
    }

    Loader {
        id: presentation

        anchors.fill: parent

        active: root.pageModel.hasProject

        sourceComponent: PresentationView {
            forceContinuousView: true

            onDoubleClicked: web.toggleFullscreen()
        }

        onLoaded: {
            prv.passed = ({})
            prv.apply(presentation.item, prv.state)
        }
    }

    //! The part of the score may come with the first state, after the score is open
    Connections {
        target: root.pageModel

        function onPartsChanged() {
            prv.apply(presentation.item, prv.state)
        }
    }
}
