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

pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents
import MuseScore.AppShell

//! Modal overlay with the audio settings (driver, device, buffer size). Covers its parent.
//! Changes apply immediately; a larger buffer size helps against stuttering playback.
FocusScope {
    id: root

    property NavigationSection navigationSection: null

    visible: false

    function open() {
        root.visible = true
        root.forceActiveFocus()
    }

    function close() {
        root.visible = false
    }

    Keys.onEscapePressed: {
        root.close()
    }

    AudioSettingsModel {
        id: audioModel
    }

    Component.onCompleted: {
        audioModel.load()
    }

    QtObject {
        id: prv

        readonly property int padding: 20
        readonly property int touchSize: 44
        readonly property int spacing: 12
    }

    NavigationPanel {
        id: navPanel

        name: "AudioSettingsDialog"
        section: root.navigationSection
        order: 1
        enabled: root.visible
        direction: NavigationPanel.Vertical
        accessible.name: titleLabel.text
    }

    //! Dims the page and blocks clicks on it; a click outside the dialog closes it
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.45)

        MouseArea {
            anchors.fill: parent
            onClicked: root.close()
        }
    }

    Rectangle {
        id: dialog

        anchors.centerIn: parent

        width: Math.min(400, root.width - 32)
        height: Math.min(contentColumn.implicitHeight + 2 * prv.padding, root.height - 32)

        radius: 8
        color: ui.theme.backgroundPrimaryColor
        border.width: ui.theme.borderWidth
        border.color: ui.theme.strokeColor

        //! Keeps clicks inside the dialog from closing it
        MouseArea {
            anchors.fill: parent
        }

        Column {
            id: contentColumn

            anchors.fill: parent
            anchors.margins: prv.padding

            spacing: prv.spacing

            StyledTextLabel {
                id: titleLabel

                width: parent.width
                horizontalAlignment: Text.AlignLeft
                font: ui.theme.largeBodyBoldFont
                text: qsTrc("appshell", "Settings")
            }

            StyledTextLabel {
                width: parent.width
                horizontalAlignment: Text.AlignLeft
                font: ui.theme.bodyBoldFont
                text: qsTrc("appshell", "Audio")
            }

            StyledTextLabel {
                width: parent.width
                horizontalAlignment: Text.AlignLeft
                visible: driverDropdown.visible
                text: qsTrc("appshell", "Audio driver")
            }

            StyledDropdown {
                id: driverDropdown

                width: parent.width
                height: prv.touchSize

                visible: audioModel.audioDrivers.length > 1

                model: audioModel.audioDrivers
                currentIndex: audioModel.currentAudioDriverIndex

                navigation.name: "AudioDriverDropdown"
                navigation.panel: navPanel
                navigation.order: 1

                onActivated: function(index, value) {
                    audioModel.selectAudioDriver(index)
                }
            }

            StyledTextLabel {
                width: parent.width
                horizontalAlignment: Text.AlignLeft
                text: qsTrc("appshell", "Audio device")
            }

            StyledDropdown {
                id: deviceDropdown

                width: parent.width
                height: prv.touchSize

                model: audioModel.deviceList
                currentIndex: deviceDropdown.indexOfValue(audioModel.currentDeviceId)

                navigation.name: "AudioDeviceDropdown"
                navigation.panel: navPanel
                navigation.order: 2

                onActivated: function(index, value) {
                    audioModel.selectDevice(value)
                }
            }

            StyledTextLabel {
                width: parent.width
                horizontalAlignment: Text.AlignLeft
                text: qsTrc("appshell", "Buffer size (samples)")
            }

            StyledDropdown {
                id: bufferDropdown

                width: parent.width
                height: prv.touchSize

                model: audioModel.bufferSizeList.map(size => ({ text: String(size), value: size }))
                currentIndex: audioModel.bufferSizeList.indexOf(audioModel.bufferSize)

                navigation.name: "BufferSizeDropdown"
                navigation.panel: navPanel
                navigation.order: 3

                onActivated: function(index, value) {
                    audioModel.selectBufferSize(value)
                }
            }

            StyledTextLabel {
                width: parent.width
                horizontalAlignment: Text.AlignLeft
                wrapMode: Text.WordWrap
                opacity: 0.7
                text: qsTrc("appshell", "If playback stutters, choose a larger buffer size.")
            }

            FlatButton {
                anchors.right: parent.right

                height: prv.touchSize
                minWidth: 132

                text: qsTrc("appshell", "Close")
                accentButton: true

                navigation.name: "CloseButton"
                navigation.panel: navPanel
                navigation.order: 1000

                onClicked: {
                    root.close()
                }
            }
        }
    }
}
