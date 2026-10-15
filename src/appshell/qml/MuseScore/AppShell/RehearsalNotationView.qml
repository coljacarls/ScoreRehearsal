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

import MuseScore.NotationScene

Item {
    id: root

    property alias name: notationView.name
    readonly property alias navigationSection: notationView.navigationSection
    property alias viewModeOverride: notationView.viewModeOverride

    //! Laser pointer mode: a tap does not seek but highlights the tapped element (and is reported)
    property bool laserActive: false
    signal laserTapped(rect logicRect)

    NotationView {
        id: notationView

        anchors.fill: parent

        readOnly: true
        tapToSeek: true
        reportTaps: root.laserActive

        onTapped: function(logicRect) {
            root.laserTapped(logicRect)
            root.logicArea = logicRect
            root.updateViewArea()
            laser.flash()
        }

        //! No score tabs: the app shows a single score and never saves it
        isSwitchPanelVisible: false
    }

    //! The highlighted area (logical score coordinates): follows the score when it is moved or zoomed
    property rect logicArea: Qt.rect(0, 0, 0, 0)
    property rect viewArea: Qt.rect(0, 0, 0, 0)

    function updateViewArea() {
        const paintView = notationView.paintView
        const rect = paintView.logicalToView(root.logicArea)
        const topLeft = paintView.mapToItem(root, rect.x, rect.y)
        root.viewArea = Qt.rect(topLeft.x, topLeft.y, rect.width, rect.height)
    }

    Connections {
        target: notationView.paintView

        function onMatrixChanged() {
            if (laser.visible) {
                root.updateViewArea()
            }
        }
    }

    LaserHighlight {
        id: laser

        area: root.viewArea
    }
}
