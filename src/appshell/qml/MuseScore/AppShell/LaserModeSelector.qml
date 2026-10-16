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

//! The menu of the laser pointer that slides out to the right of its button: the mode of the pointer
//! (0 = rectangular selection, 1 = pen)
Item {
    id: root

    property bool expanded: false
    property int mode: 0
    property int buttonSize: 36

    property NavigationPanel navigationPanel: null
    property int navigationOrderStart: 1

    //! The same gold as the solo slider and the laser pointer button
    readonly property color goldColor: "#E0B23A"

    signal modeSelected(int mode)

    width: root.expanded ? pill.width : 0
    height: root.buttonSize

    visible: width > 0
    opacity: root.expanded ? 1 : 0
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

    //! A pill with a gold outline around the two options: they belong to the laser pointer button next to it
    Rectangle {
        id: pill

        width: row.implicitWidth + 2 * 3
        height: root.buttonSize

        radius: height / 2
        color: Qt.rgba(root.goldColor.r, root.goldColor.g, root.goldColor.b, 0.12)
        border.width: 1
        border.color: root.goldColor

        Row {
            id: row

            anchors.centerIn: parent
            spacing: 2

            FlatButton {
                id: rectButton

                width: root.buttonSize - 8
                height: root.buttonSize - 8
                backgroundRadius: height / 2

                accentButton: root.mode === 0
                accentColor: root.goldColor
                transparent: root.mode !== 0
                iconColor: root.mode === 0 ? "#1E1E1E" : ui.theme.fontPrimaryColor

                toolTipTitle: qsTrc("appshell", "Rectangle")
                toolTipDescription: qsTrc("appshell", "Tapping frames the tapped notes")
                accessible.checked: root.mode === 0

                navigation.panel: root.navigationPanel
                navigation.order: root.navigationOrderStart

                contentItem: Component {
                    //! A rectangular selection: dashed rectangle
                    Canvas {
                        implicitWidth: 22
                        implicitHeight: 22

                        property color color: rectButton.iconColor

                        onColorChanged: requestPaint()

                        onPaint: {
                            const ctx = getContext("2d")
                            ctx.reset()
                            ctx.strokeStyle = color
                            ctx.lineWidth = 2
                            ctx.setLineDash([3, 2.5])
                            ctx.strokeRect(3, 5, width - 6, height - 10)
                        }
                    }
                }

                onClicked: {
                    root.modeSelected(0)
                }
            }

            FlatButton {
                width: root.buttonSize - 8
                height: root.buttonSize - 8
                backgroundRadius: height / 2

                icon: IconCode.EDIT
                accentButton: root.mode === 1
                accentColor: root.goldColor
                transparent: root.mode !== 1
                iconColor: root.mode === 1 ? "#1E1E1E" : ui.theme.fontPrimaryColor

                toolTipTitle: qsTrc("appshell", "Pen")
                toolTipDescription: qsTrc("appshell", "Draw on the score with finger or mouse")
                accessible.checked: root.mode === 1

                navigation.panel: root.navigationPanel
                navigation.order: root.navigationOrderStart + 1

                onClicked: {
                    root.modeSelected(1)
                }
            }
        }
    }
}
