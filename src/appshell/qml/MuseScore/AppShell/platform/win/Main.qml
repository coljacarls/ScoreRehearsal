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
import Muse.Interactive

import MuseScore.AppShell

AppWindow {
    id: root

    function toggleMaximized() {
        if (root.visibility === Window.Maximized) {
            root.showNormal()
        } else {
            root.showMaximized()
        }
    }

    //! The state of the window (windowed with its size and position, maximized, full screen) is remembered
    //! between the runs. The geometry is the one of the windowed state, also while maximized or in full screen.
    WindowStateModel {
        id: windowState
    }

    QtObject {
        id: windowPrv

        property rect normalGeometry: Qt.rect(0, 0, 0, 0)
        property bool maximizedBeforeFullScreen: false

        function overlap(a, b) {
            let w = Math.min(a.x + a.width, b.x + b.width) - Math.max(a.x, b.x)
            let h = Math.min(a.y + a.height, b.y + b.height) - Math.max(a.y, b.y)
            return Qt.size(Math.max(0, w), Math.max(0, h))
        }

        //! Whether enough of the window is on a screen to grab and move it (screens may be gone since the last run)
        function isReachable(rect) {
            for (let i = 0; i < Qt.application.screens.length; ++i) {
                let s = Qt.application.screens[i]
                let o = overlap(rect, Qt.rect(s.virtualX, s.virtualY, s.width, s.height))
                if (o.width >= 100 && o.height >= 50) {
                    return true
                }
            }
            return false
        }

        function restore() {
            let g = windowState.geometry
            let valid = g.width >= root.minimumWidth && g.height >= root.minimumHeight && isReachable(g)
            if (valid) {
                root.x = g.x
                root.y = g.y
                root.width = g.width
                root.height = g.height
            }

            normalGeometry = Qt.rect(root.x, root.y, root.width, root.height)

            let state = windowState.state
            if (state === "maximized") {
                root.visibility = Window.Maximized
            } else if (state === "fullscreen") {
                maximizedBeforeFullScreen = windowState.maximizedBeforeFullScreen
                root.visibility = maximizedBeforeFullScreen ? Window.Maximized : Window.Windowed
                Qt.callLater(windowState.enterFullScreen)
            }
        }

        function save() {
            let state
            switch (root.visibility) {
            case Window.FullScreen:
                state = "fullscreen"
                break
            case Window.Maximized:
                state = "maximized"
                maximizedBeforeFullScreen = true
                break
            case Window.Windowed:
                state = "windowed"
                maximizedBeforeFullScreen = false
                //! Only the windowed state tells the geometry
                normalGeometry = Qt.rect(root.x, root.y, root.width, root.height)
                break
            default:
                return
            }

            windowState.save(state, Qt.rect(normalGeometry.x, normalGeometry.y, normalGeometry.width, normalGeometry.height),
                             maximizedBeforeFullScreen)
        }
    }

    //! Moving and resizing come as many changes, and maximizing resizes the window before it reports its new state
    Timer {
        id: saveTimer

        interval: 400
        onTriggered: windowPrv.save()
    }

    onXChanged: saveTimer.restart()
    onYChanged: saveTimer.restart()
    onWidthChanged: saveTimer.restart()
    onHeightChanged: saveTimer.restart()
    onVisibilityChanged: saveTimer.restart()

    onClosing: windowPrv.save()

    Component.onCompleted: windowPrv.restore()

    //! Registers the title bar as the area that moves the (frameless) window
    WindowsModel {
        id: windowsModel

        mainWindowTitleBarMoveArea: appTitleBar.titleMoveAreaRect
    }

    //! The shell has a single page. Every page URI the MuseScore modules open
    //! (home, notation, rehearsal) is answered with it.
    InteractiveProvider {
        id: interactiveProvider
        topParent: root

        onRequestedDockPage: function(uri, params) {
            Qt.callLater(interactiveProvider.onPageOpened)
        }
    }

    AppTitleBar {
        id: appTitleBar

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        height: 32
        title: root.title

        //! Hidden in full screen (and so in the immersive mode of the page)
        visible: root.visibility !== Window.FullScreen

        windowVisibility: root.visibility

        onShowWindowMinimizedRequested: {
            root.showMinimizedWithSavePreviousState()
        }

        onToggleWindowMaximizedRequested: {
            root.toggleMaximized()
        }

        onCloseWindowRequested: {
            Qt.callLater(root.close)
        }
    }

    RehearsalPage {
        anchors.top: appTitleBar.visible ? appTitleBar.bottom : parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }
}
