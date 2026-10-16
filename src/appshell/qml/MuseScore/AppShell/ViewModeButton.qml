/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited and others
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
import Muse.UiComponents
//! Select button for the view of the score: page view or continuous view
FlatButton {
    id: root

    required property RehearsalPageModel pageModel

    property NavigationPanel navigationPanel: null
    property int navigationOrder: 1
    property int buttonSize: 36
    readonly property font iconFont: Qt.font({ family: ui.theme.iconsFont.family, pixelSize: root.buttonSize >= 40 ? 24 : 20 })

    height: root.buttonSize
    width: Math.max(root.buttonSize, implicitWidth)

    transparent: !viewModeMenu.isMenuOpened
    accentButton: viewModeMenu.isMenuOpened
    orientation: Qt.Horizontal
    buttonType: FlatButton.Horizontal

    icon: root.pageModel.isContinuousView ? IconCode.CONTINUOUS_VIEW : IconCode.PAGE_VIEW
    toolTipTitle: root.pageModel.isContinuousView ? qsTrc("appshell", "Continuous view (horizontal)")
                                                  : qsTrc("appshell", "Page view")

    navigation.panel: root.navigationPanel
    navigation.order: root.navigationOrder

    contentItem: Row {
        spacing: 2

        StyledIconLabel {
            anchors.verticalCenter: parent.verticalCenter
            iconCode: root.icon
            font: root.iconFont
        }

        StyledIconLabel {
            anchors.verticalCenter: parent.verticalCenter
            iconCode: IconCode.SMALL_ARROW_DOWN
        }
    }

    onClicked: {
        viewModeMenu.toggleOpened([
            { id: "page", title: qsTrc("appshell", "Page view"), icon: IconCode.PAGE_VIEW,
                checkable: true, checked: !root.pageModel.isContinuousView },
            { id: "continuous", title: qsTrc("appshell", "Continuous view (horizontal)"), icon: IconCode.CONTINUOUS_VIEW,
                checkable: true, checked: root.pageModel.isContinuousView }
        ])
    }

    StyledMenuLoader {
        id: viewModeMenu

        menuAnchorItem: root.Window.window ? root.Window.window.contentItem : null

        onHandleMenuItem: function(itemId) {
            root.pageModel.isContinuousView = (itemId === "continuous")
        }
    }
}
