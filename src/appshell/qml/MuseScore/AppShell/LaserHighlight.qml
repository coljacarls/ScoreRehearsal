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
import QtQuick.Effects
import QtQuick.Shapes

//! The laser pointer highlight: a frame around an area of the score (view coordinates) whose outline
//! is a rainbow gradient that turns around the frame, with a soft glow of the same colors.
//! It stays as long as `shown` is true and fades in and out.
Item {
    id: root

    //! The area to highlight; the frame is drawn around it
    property rect area: Qt.rect(0, 0, 0, 0)
    property bool shown: false

    readonly property real padding: 8
    readonly property real lineWidth: 3.5
    readonly property real cornerRadius: 8
    readonly property int rotationDuration: 2400
    readonly property int fadeDuration: 250

    x: area.x - padding
    y: area.y - padding
    width: area.width + 2 * padding
    height: area.height + 2 * padding

    opacity: root.shown ? 1 : 0
    visible: opacity > 0
    enabled: false

    Behavior on opacity {
        NumberAnimation {
            duration: root.fadeDuration
        }
    }

    //! The turning rainbow ring
    Item {
        id: ring

        anchors.fill: parent
        layer.enabled: true
        layer.smooth: true

        Shape {
            anchors.fill: parent
            preferredRendererType: Shape.CurveRenderer

            ShapePath {
                fillRule: ShapePath.OddEvenFill
                strokeColor: "transparent"

                fillGradient: ConicalGradient {
                    id: rainbow

                    centerX: ring.width / 2
                    centerY: ring.height / 2

                    GradientStop { position: 0.00; color: "#FF4D4D" }
                    GradientStop { position: 0.17; color: "#FFB020" }
                    GradientStop { position: 0.33; color: "#E8E83A" }
                    GradientStop { position: 0.50; color: "#3DDC84" }
                    GradientStop { position: 0.67; color: "#33B5FF" }
                    GradientStop { position: 0.83; color: "#9B5CFF" }
                    GradientStop { position: 1.00; color: "#FF4D4D" }

                    NumberAnimation on angle {
                        running: root.visible
                        from: 0
                        to: 360
                        duration: root.rotationDuration
                        loops: Animation.Infinite
                    }
                }

                PathRectangle {
                    x: 0
                    y: 0
                    width: ring.width
                    height: ring.height
                    radius: root.cornerRadius
                }

                PathRectangle {
                    x: root.lineWidth
                    y: root.lineWidth
                    width: ring.width - 2 * root.lineWidth
                    height: ring.height - 2 * root.lineWidth
                    radius: Math.max(root.cornerRadius - root.lineWidth, 0)
                }
            }
        }
    }

    //! The glow: the ring again, blurred, behind it
    MultiEffect {
        anchors.fill: ring
        z: -1

        source: ring
        autoPaddingEnabled: true
        blurEnabled: true
        blurMax: 48
        blur: 1.0
        brightness: 0.1
        saturation: 0.3
    }
}
