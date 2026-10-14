/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
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

import QtQuick

import Muse.Ui
import Muse.UiComponents

//! Minimize / maximize / close buttons of the custom title bar (the ui module removes
//! the native window frame of the main window on Windows)
Row {
    id: root

    property bool windowIsMaximized: false

    spacing: 8

    signal showWindowMinimizedRequested()
    signal toggleWindowMaximizedRequested()
    signal closeWindowRequested()

    FlatButton {
        id: minimizeButton

        icon: IconCode.APP_MINIMIZE
        transparent: true
        drawFocusBorderInsideRect: true

        onClicked: {
            root.showWindowMinimizedRequested()
        }
    }

    FlatButton {
        id: maximizeButton

        icon: !root.windowIsMaximized ? IconCode.APP_MAXIMIZE : IconCode.APP_UNMAXIMIZE
        transparent: true
        drawFocusBorderInsideRect: true

        onClicked: {
            root.toggleWindowMaximizedRequested()
        }
    }

    FlatButton {
        id: closeButton

        icon: IconCode.APP_CLOSE
        transparent: true
        drawFocusBorderInsideRect: true

        onClicked: {
            root.closeWindowRequested()
        }
    }
}
