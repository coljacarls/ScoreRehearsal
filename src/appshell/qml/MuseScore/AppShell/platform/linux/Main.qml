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
import Muse.Interactive

import MuseScore.AppShell

AppWindow {
    id: root

    //! The shell has a single page. Every page URI the MuseScore modules open
    //! (home, notation, rehearsal) is answered with it.
    InteractiveProvider {
        id: interactiveProvider
        topParent: root

        onRequestedDockPage: function(uri, params) {
            Qt.callLater(interactiveProvider.onPageOpened)
        }
    }

    //! The browser version opens a second tab for the audience: it shows the presentation page instead
    WebBridge {
        id: web
    }

    Loader {
        anchors.fill: parent

        sourceComponent: web.isPresentationInstance ? presentationPage : rehearsalPage
    }

    Component {
        id: rehearsalPage

        RehearsalPage {
            anchors.fill: parent
        }
    }

    Component {
        id: presentationPage

        PresentationPage {
            anchors.fill: parent
        }
    }
}
