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

//! Button of the laser pointer mode. While the mode is active it is filled with the gold of the solo slider
//! (and not with the blue accent color), so that it can be seen at first glance.
FlatButton {
    id: root

    property bool active: false
    property int iconSize: 22

    //! The same gold as the solo slider and frames in the tracks panel
    readonly property color goldColor: "#E0B23A"

    accentButton: root.active
    accentColor: root.goldColor
    transparent: !root.active

    toolTipTitle: root.active ? qsTrc("appshell", "Laser pointer off") : qsTrc("appshell", "Laser pointer")
    toolTipDescription: qsTrc("appshell", "Tapping the score highlights the tapped notes in both views")
    accessible.checked: root.active

    //! Dark on the gold, the usual icon color otherwise
    iconColor: root.active ? "#1E1E1E" : ui.theme.fontPrimaryColor

    contentItem: Component {
        Canvas {
            id: canvas

            implicitWidth: root.iconSize
            implicitHeight: root.iconSize

            property color color: root.iconColor
            onColorChanged: requestPaint()
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()

            //! A pointer pen (bottom left) that sends a ray to a lit dot (top right)
            onPaint: {
                const ctx = getContext("2d")
                ctx.reset()

                const s = Math.min(width, height) / 24
                ctx.scale(s, s)
                ctx.lineCap = "round"
                ctx.strokeStyle = canvas.color
                ctx.fillStyle = canvas.color

                //! The body
                ctx.save()
                ctx.translate(7, 17)
                ctx.rotate(-Math.PI / 4)
                ctx.beginPath()
                ctx.roundedRect(-8, -2.2, 12, 4.4, 2, 2)
                ctx.fill()
                ctx.restore()

                //! The beam
                ctx.lineWidth = 1.4
                ctx.setLineDash([1.5, 2.2])
                ctx.beginPath()
                ctx.moveTo(10.5, 13.5)
                ctx.lineTo(16, 8)
                ctx.stroke()
                ctx.setLineDash([])

                //! The lit dot with its rays
                ctx.beginPath()
                ctx.arc(18.5, 5.5, 2.2, 0, 2 * Math.PI)
                ctx.fill()

                ctx.lineWidth = 1.5
                for (let i = 0; i < 8; ++i) {
                    const a = i * Math.PI / 4
                    ctx.beginPath()
                    ctx.moveTo(18.5 + Math.cos(a) * 3.6, 5.5 + Math.sin(a) * 3.6)
                    ctx.lineTo(18.5 + Math.cos(a) * 5.2, 5.5 + Math.sin(a) * 5.2)
                    ctx.stroke()
                }
            }
        }
    }
}
