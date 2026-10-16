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

//! The floating panel with the tools over the score (top left): the hand (move the score, tap to place the
//! playhead), the laser pointer and - while the laser pointer is selected - its tools: rectangular selection,
//! pen, select/edit annotations and delete all annotations.
Rectangle {
    id: root

    required property RehearsalPageModel pageModel

    property NavigationSection navigationSection: null
    property int navigationOrderStart: 1

    readonly property int buttonSize: 40
    readonly property bool laserActive: root.pageModel.isLaserPointerActive

    //! The same gold as the solo slider and the laser pointer button
    readonly property color goldColor: "#E0B23A"

    implicitWidth: row.implicitWidth + 12 * 2
    implicitHeight: root.buttonSize + 12
    width: implicitWidth
    height: implicitHeight
    radius: height / 2

    color: Utils.colorWithAlpha(ui.theme.backgroundPrimaryColor, 0.9)
    border.color: ui.theme.strokeColor
    border.width: ui.theme.borderWidth

    //! Do not let clicks on the panel reach the score
    MouseArea {
        anchors.fill: parent
    }

    NavigationPanel {
        id: navPanel
        name: "RehearsalTools"
        enabled: root.enabled && root.visible
        section: root.navigationSection
        order: root.navigationOrderStart
        direction: NavigationPanel.Horizontal
        accessible.name: qsTrc("appshell", "Tools")
    }

    Row {
        id: row

        anchors.centerIn: parent
        spacing: 4

        RehearsalToolButton {
            width: root.buttonSize
            height: root.buttonSize

            kind: "hand"
            gold: false
            selected: !root.laserActive

            toolTipTitle: qsTrc("appshell", "Hand")
            toolTipDescription: qsTrc("appshell", "Move the score, tap to place the playhead")

            navigation.panel: navPanel
            navigation.order: 1

            onClicked: {
                root.pageModel.isLaserPointerActive = false
            }
        }

        LaserPointerButton {
            width: root.buttonSize
            height: root.buttonSize
            iconSize: 26

            active: root.laserActive

            navigation.panel: navPanel
            navigation.order: 2

            onClicked: {
                root.pageModel.isLaserPointerActive = true
            }
        }

        //! The tools of the laser pointer slide out to the right of its button
        Item {
            id: tools

            width: root.laserActive ? toolsRow.implicitWidth : 0
            height: root.buttonSize

            visible: width > 0
            opacity: root.laserActive ? 1 : 0
            clip: true

            Behavior on width {
                NumberAnimation {
                    duration: 180
                    easing.type: Easing.OutCubic
                }
            }

            Behavior on opacity {
                NumberAnimation {
                    duration: 180
                }
            }

            Row {
                id: toolsRow

                spacing: 4

                //! Separates the tools of the laser pointer
                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 1
                    height: root.buttonSize * 0.6
                    color: root.goldColor
                }

                RehearsalToolButton {
                    width: root.buttonSize
                    height: root.buttonSize

                    kind: "rect"
                    selected: root.pageModel.laserMode === 0

                    toolTipTitle: qsTrc("appshell", "Rectangle")
                    toolTipDescription: qsTrc("appshell", "Tapping frames the tapped notes")

                    navigation.panel: navPanel
                    navigation.order: 3

                    onClicked: {
                        root.pageModel.laserMode = 0
                    }
                }

                RehearsalToolButton {
                    width: root.buttonSize
                    height: root.buttonSize

                    icon: IconCode.EDIT
                    iconFont: Qt.font({ family: ui.theme.iconsFont.family, pixelSize: 24 })
                    selected: root.pageModel.laserMode === 1

                    toolTipTitle: qsTrc("appshell", "Pen")
                    toolTipDescription: qsTrc("appshell", "Draw on the score with finger or mouse")

                    navigation.panel: navPanel
                    navigation.order: 4

                    onClicked: {
                        root.pageModel.laserMode = 1
                    }
                }

                RehearsalToolButton {
                    width: root.buttonSize
                    height: root.buttonSize

                    kind: "point"
                    selected: root.pageModel.laserMode === 2

                    toolTipTitle: qsTrc("appshell", "Select")
                    toolTipDescription: qsTrc("appshell", "Select a frame or a stroke to change or delete it")

                    navigation.panel: navPanel
                    navigation.order: 5

                    onClicked: {
                        root.pageModel.laserMode = 2
                    }
                }

                RehearsalToolButton {
                    width: root.buttonSize
                    height: root.buttonSize

                    icon: IconCode.DELETE_TANK
                    iconFont: Qt.font({ family: ui.theme.iconsFont.family, pixelSize: 24 })
                    enabled: root.pageModel.hasHighlight || root.pageModel.strokes.length > 0

                    toolTipTitle: qsTrc("appshell", "Delete all annotations")

                    navigation.panel: navPanel
                    navigation.order: 6

                    onClicked: {
                        root.pageModel.askClearAnnotations()
                    }
                }
            }
        }
    }
}
