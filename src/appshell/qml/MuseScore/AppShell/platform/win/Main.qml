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
