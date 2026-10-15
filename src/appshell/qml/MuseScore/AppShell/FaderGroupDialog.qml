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

//! Modal overlay to create a fader group: choose the tracks, name the group, confirm.
//! Covers its parent; open() fills it from the mixer model.
FocusScope {
    id: root

    property RehearsalMixerModel mixerModel: null

    property NavigationSection navigationSection: null

    visible: false

    function open() {
        if (!root.mixerModel) {
            return
        }

        prv.tracks = root.mixerModel.ungroupedTracks()
        prv.selectedKeys = []
        prv.name = root.mixerModel.suggestedGroupName()

        root.visible = true
        root.forceActiveFocus()
        nameField.ensureActiveFocus()
        nameField.selectAll()
    }

    function close() {
        root.visible = false
    }

    function accept() {
        if (!prv.canCreate) {
            return
        }

        if (root.mixerModel.createGroup(prv.name, prv.selectedKeys)) {
            root.close()
        }
    }

    Keys.onEscapePressed: {
        root.close()
    }

    QtObject {
        id: prv

        property var tracks: []
        property var selectedKeys: []
        property string name: ""

        readonly property bool canCreate: prv.name.trim().length > 0 && prv.selectedKeys.length > 0

        function isSelected(trackKey) {
            return prv.selectedKeys.indexOf(trackKey) !== -1
        }

        function toggle(trackKey) {
            const keys = prv.selectedKeys.slice()
            const position = keys.indexOf(trackKey)

            if (position === -1) {
                keys.push(trackKey)
            } else {
                keys.splice(position, 1)
            }

            prv.selectedKeys = keys
        }

        function selectAll(select) {
            prv.selectedKeys = select ? prv.tracks.map(track => track.trackKey) : []
        }
    }

    NavigationPanel {
        id: navPanel

        name: "FaderGroupDialog"
        section: root.navigationSection
        order: 1
        enabled: root.visible
        direction: NavigationPanel.Vertical
        accessible.name: titleLabel.text
    }

    //! Dims the page and blocks clicks on it; a click outside the dialog cancels
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
        height: Math.min(prv_layout.fixedHeight + tracksColumn.height + 2 * prv_layout.padding, root.height - 32)

        radius: 8
        color: ui.theme.backgroundPrimaryColor
        border.width: ui.theme.borderWidth
        border.color: ui.theme.strokeColor

        //! Keeps clicks inside the dialog from closing it
        MouseArea {
            anchors.fill: parent
        }

        QtObject {
            id: prv_layout

            readonly property int padding: 20
            readonly property int touchSize: 44
            readonly property int spacing: 12

            //! Height of everything except the track list
            readonly property real fixedHeight: titleLabel.height + nameLabel.height + nameField.height
                                                + tracksHeader.height + buttonsRow.height + 5 * prv_layout.spacing
        }

        Column {
            id: contentColumn

            anchors.fill: parent
            anchors.margins: prv_layout.padding

            spacing: prv_layout.spacing

            StyledTextLabel {
                id: titleLabel

                width: parent.width
                horizontalAlignment: Text.AlignLeft
                font: ui.theme.largeBodyBoldFont
                text: qsTrc("appshell", "New fader group")
            }

            StyledTextLabel {
                id: nameLabel

                width: parent.width
                horizontalAlignment: Text.AlignLeft
                text: qsTrc("appshell", "Name")
            }

            TextInputField {
                id: nameField

                width: parent.width
                height: prv_layout.touchSize

                currentText: prv.name
                maximumLength: 40

                navigation.name: "GroupNameField"
                navigation.panel: navPanel
                navigation.order: 1
                accessible.name: qsTrc("appshell", "Name")

                onTextChanged: function(newTextValue) {
                    prv.name = newTextValue
                }

                onAccepted: {
                    root.accept()
                }

                onEscaped: {
                    root.close()
                }
            }

            Item {
                id: tracksHeader

                width: parent.width
                height: 28

                StyledTextLabel {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter

                    horizontalAlignment: Text.AlignLeft
                    text: qsTrc("appshell", "Tracks in the group")
                }

                CheckBox {
                    id: allCheckBox

                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter

                    text: qsTrc("appshell", "All")
                    checked: prv.tracks.length > 0 && prv.selectedKeys.length === prv.tracks.length
                    isIndeterminate: prv.selectedKeys.length > 0 && prv.selectedKeys.length < prv.tracks.length

                    navigation.name: "SelectAllCheckBox"
                    navigation.panel: navPanel
                    navigation.order: 2

                    onClicked: {
                        prv.selectAll(!allCheckBox.checked)
                    }
                }
            }

            StyledFlickable {
                id: tracksFlickable

                width: parent.width
                height: Math.max(0, dialog.height - 2 * prv_layout.padding - prv_layout.fixedHeight)

                contentHeight: tracksColumn.height
                clip: true

                Column {
                    id: tracksColumn

                    width: parent.width

                    Repeater {
                        model: prv.tracks

                        delegate: CheckBox {
                            id: trackCheckBox

                            required property var modelData
                            required property int index

                            width: tracksColumn.width
                            height: prv_layout.touchSize

                            text: trackCheckBox.modelData.title
                            checked: prv.isSelected(trackCheckBox.modelData.trackKey)

                            navigation.name: "TrackCheckBox" + trackCheckBox.index
                            navigation.panel: navPanel
                            navigation.order: 3 + trackCheckBox.index

                            onClicked: {
                                prv.toggle(trackCheckBox.modelData.trackKey)
                            }
                        }
                    }
                }
            }

            Row {
                id: buttonsRow

                anchors.right: parent.right

                spacing: 8

                FlatButton {
                    height: prv_layout.touchSize
                    minWidth: 132

                    text: qsTrc("appshell", "Cancel")

                    navigation.name: "CancelButton"
                    navigation.panel: navPanel
                    navigation.order: 1000

                    onClicked: {
                        root.close()
                    }
                }

                FlatButton {
                    height: prv_layout.touchSize
                    minWidth: 132

                    text: qsTrc("appshell", "Create")
                    accentButton: true
                    enabled: prv.canCreate

                    navigation.name: "CreateButton"
                    navigation.panel: navPanel
                    navigation.order: 1001

                    onClicked: {
                        root.accept()
                    }
                }
            }
        }
    }
}
