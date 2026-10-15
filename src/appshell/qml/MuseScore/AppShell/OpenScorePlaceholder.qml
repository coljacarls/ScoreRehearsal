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

//! Shown while no score is open
Item {
    id: root

    signal openRequested()
    signal openSessionRequested(string sessionId)
    signal removeSessionRequested(string sessionId)

    //! The files can be moved or deleted while the app is running
    onVisibleChanged: {
        if (visible) {
            sessionsModel.refresh()
        }
    }

    NavigationPanel {
        id: navPanel
        name: "OpenScorePlaceholder"
        enabled: root.enabled && root.visible
        order: 3
        direction: NavigationPanel.Vertical
        accessible.name: qsTrc("appshell", "Open score")
    }

    AboutModel {
        id: aboutModel
    }

    RehearsalSessionListModel {
        id: sessionsModel
    }

    Column {
        anchors.centerIn: parent
        width: Math.min(480, parent.width - 48)

        spacing: 16

        Image {
            anchors.horizontalCenter: parent.horizontalCenter

            source: "resources/scorerehearsal_logo.png"
            sourceSize: Qt.size(96, 96)
            width: 96
            height: 96
        }

        StyledTextLabel {
            width: parent.width
            text: "ScoreRehearsal"
            font: ui.theme.headerBoldFont
            horizontalAlignment: Text.AlignHCenter
        }

        StyledTextLabel {
            width: parent.width
            text: qsTrc("appshell", "Open a MuseScore file (.mscz, .mscx) to rehearse with it, or drop it onto this window. The file is never changed.")
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        FlatButton {
            anchors.horizontalCenter: parent.horizontalCenter
            height: 40

            icon: IconCode.OPEN_FILE
            text: qsTrc("appshell", "Open score…")
            orientation: Qt.Horizontal
            accentButton: true

            navigation.panel: navPanel
            navigation.order: 1

            onClicked: {
                root.openRequested()
            }
        }

        StyledTextLabel {
            width: parent.width
            visible: sessionsModel.count > 0
            text: qsTrc("appshell", "Sessions")
            font: ui.theme.largeBodyBoldFont
            horizontalAlignment: Text.AlignLeft
        }

        StyledListView {
            id: sessionsView

            width: parent.width
            height: Math.min(contentHeight, 5 * 56)
            visible: sessionsModel.count > 0

            model: sessionsModel
            spacing: 0

            delegate: ListItemBlank {
                id: sessionItem

                required property int index
                required property string sessionId
                required property string title
                required property string filePath
                required property bool fileExists
                required property string lastOpened

                height: 56
                width: sessionsView.width

                navigation.panel: navPanel
                navigation.order: 2 + index * 2
                navigation.accessible.name: title

                onClicked: {
                    root.openSessionRequested(sessionId)
                }

                Column {
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.right: removeButton.left
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter

                    StyledTextLabel {
                        width: parent.width
                        text: sessionItem.title
                        font: ui.theme.bodyBoldFont
                        horizontalAlignment: Text.AlignLeft
                        elide: Text.ElideRight
                        opacity: sessionItem.fileExists ? 1.0 : 0.6
                    }

                    StyledTextLabel {
                        width: parent.width
                        text: sessionItem.fileExists
                              ? sessionItem.lastOpened
                              : qsTrc("appshell", "File not found: %1").arg(sessionItem.filePath)
                        horizontalAlignment: Text.AlignLeft
                        elide: Text.ElideMiddle
                        opacity: 0.6
                    }
                }

                FlatButton {
                    id: removeButton

                    anchors.right: parent.right
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    width: 36
                    height: 36

                    icon: IconCode.DELETE_TANK
                    transparent: true

                    toolTipTitle: qsTrc("appshell", "Delete session")

                    navigation.panel: navPanel
                    navigation.order: 3 + sessionItem.index * 2
                    navigation.accessible.name: qsTrc("appshell", "Delete session %1").arg(sessionItem.title)

                    onClicked: {
                        root.removeSessionRequested(sessionItem.sessionId)
                    }
                }
            }
        }
    }

    StyledTextLabel {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 12

        text: qsTrc("appshell", "Version %1").arg(aboutModel.appVersion())
        font: ui.theme.bodyFont
        opacity: 0.6
    }
}
