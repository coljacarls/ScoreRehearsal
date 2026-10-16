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

//! A tool button of the rehearsal tools panel. The icon is one of the app's own drawings (kind: "hand",
//! "point", "rect") or an icon of the icon font (iconCode). The selected tool is filled with the accent color.
FlatButton {
    id: root

    //! "hand", "point", "rect", "expand", "collapse" or "" (then iconCode is used)
    property string kind: ""
    property bool selected: false
    property int iconSize: 22

    //! The gold of the laser pointer tools
    readonly property color goldColor: "#E0B23A"
    property bool gold: true

    accentButton: root.selected
    accentColor: root.gold ? root.goldColor : ui.theme.accentColor
    transparent: !root.selected
    backgroundRadius: height / 2

    //! Dark on the gold, the usual icon color otherwise
    iconColor: root.selected ? (root.gold ? "#1E1E1E" : "white") : ui.theme.fontPrimaryColor
    accessible.checked: root.selected

    contentItem: root.kind === "" ? null : drawnIcon

    Component {
        id: drawnIcon

        Canvas {
            implicitWidth: root.iconSize
            implicitHeight: root.iconSize

            property color color: root.iconColor
            onColorChanged: requestPaint()
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()

            onPaint: {
                const ctx = getContext("2d")
                ctx.reset()

                const s = Math.min(width, height) / 24
                ctx.scale(s, s)
                ctx.fillStyle = color
                ctx.strokeStyle = color
                ctx.lineCap = "round"
                ctx.lineJoin = "round"

                if (root.kind === "rect") {
                    //! A rectangular selection: dashed rectangle
                    ctx.lineWidth = 2
                    ctx.setLineDash([3, 2.5])
                    ctx.strokeRect(3, 5, 18, 14)
                } else if (root.kind === "expand" || root.kind === "collapse") {
                    //! Expansion symbol: a rectangle with arrows in the top right and bottom left corner,
                    //! pointing out (expand) or in (collapse)
                    ctx.lineWidth = 2
                    ctx.beginPath(); ctx.roundedRect(5, 7, 14, 10, 2, 2); ctx.stroke()

                    const out = root.kind === "expand"
                    // [tip x, tip y, direction of the head arms (towards the center of the rectangle in x and y)]
                    const arrows = out ? [[21, 3, -1, 1, 14, 10], [3, 21, 1, -1, 10, 14]]
                                       : [[15, 9, 1, -1, 21, 3], [9, 15, -1, 1, 3, 21]]
                    for (const a of arrows) {
                        ctx.beginPath()
                        ctx.moveTo(a[4], a[5]); ctx.lineTo(a[0], a[1])
                        ctx.moveTo(a[0] + a[2] * 5, a[1]); ctx.lineTo(a[0], a[1]); ctx.lineTo(a[0], a[1] + a[3] * 5)
                        ctx.stroke()
                    }
                } else if (root.kind === "hand") {
                    //! A grabbing hand: the palm with four fingers and the thumb
                    ctx.beginPath(); ctx.roundedRect(6.5, 11, 12, 9, 4, 4); ctx.fill()
                    ctx.beginPath(); ctx.roundedRect(6.5, 6.5, 3, 9, 1.5, 1.5); ctx.fill()
                    ctx.beginPath(); ctx.roundedRect(10, 4, 3, 10, 1.5, 1.5); ctx.fill()
                    ctx.beginPath(); ctx.roundedRect(13.5, 5, 3, 9, 1.5, 1.5); ctx.fill()
                    ctx.beginPath(); ctx.roundedRect(16.5, 8, 3, 8, 1.5, 1.5); ctx.fill()
                    ctx.lineWidth = 3
                    ctx.beginPath(); ctx.moveTo(7.5, 16); ctx.lineTo(4, 12.5); ctx.stroke()
                } else if (root.kind === "point") {
                    //! A pointing hand: the index finger is stretched out, the other fingers are folded
                    ctx.beginPath(); ctx.roundedRect(7, 12, 12, 9, 4, 4); ctx.fill()
                    ctx.beginPath(); ctx.roundedRect(8.5, 2.5, 3.5, 13, 1.75, 1.75); ctx.fill()
                    ctx.beginPath(); ctx.roundedRect(12, 10, 3, 5, 1.5, 1.5); ctx.fill()
                    ctx.beginPath(); ctx.roundedRect(15, 10.5, 3, 5, 1.5, 1.5); ctx.fill()
                    ctx.lineWidth = 3
                    ctx.beginPath(); ctx.moveTo(8, 16.5); ctx.lineTo(5, 14); ctx.stroke()
                }
            }
        }
    }
}
