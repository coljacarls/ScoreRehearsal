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

//! Modal overlay to set the playback instrument of the voices: pick a voice (or a fader group)
//! on the left, then drill down to the instrument in columns (like the column view of a file browser).
//! The first column starts with the favorites, followed by the categories.
FocusScope {
    id: root

    property RehearsalMixerModel mixerModel: null

    property NavigationSection navigationSection: null

    visible: false

    function open() {
        if (!root.mixerModel) {
            return
        }

        prv.selectedTarget = -1
        prv.rebuildColumns()

        root.visible = true
        root.forceActiveFocus()
    }

    function close() {
        root.visible = false
    }

    Keys.onEscapePressed: {
        root.close()
    }

    QtObject {
        id: prv

        readonly property int touchSize: 44
        readonly property int padding: 20
        readonly property int columnWidth: 230
        readonly property color selectionColor: ui.theme.accentColor

        readonly property var targets: root.mixerModel ? root.mixerModel.instrumentTargets : []
        readonly property var favoriteKeys: root.mixerModel ? root.mixerModel.favoriteInstrumentKeys : []

        //! Index in targets of the voice or group the instrument is set for
        property int selectedTarget: -1

        readonly property var target: prv.selectedTarget >= 0 && prv.selectedTarget < prv.targets.length
                                      ? prv.targets[prv.selectedTarget] : null

        //! The favorites (if there are any) and the categories of the tree
        function rootNodes() {
            const nodes = []
            const favorites = root.mixerModel.favoriteInstruments

            if (favorites.length > 0) {
                nodes.push({ title: qsTrc("appshell", "Favorites"), subtitle: "", key: "", children: favorites })
            }

            return nodes.concat(root.mixerModel.instrumentTree)
        }

        //! Fills the first column and opens the path to the instrument of the target
        function rebuildColumns() {
            columnsModel.clear()
            columnsModel.append({ nodes: prv.rootNodes(), selected: -1 })

            if (!prv.target || prv.target.instrumentKey === "") {
                return
            }

            const path = root.mixerModel.instrumentPath(prv.target.instrumentKey)
            const offset = columnsModel.get(0).nodes.length - root.mixerModel.instrumentTree.length

            for (let level = 0; level < path.length; ++level) {
                const index = path[level] + (level === 0 ? offset : 0)
                prv.select(level, index)
            }
        }

        //! Selects a node of a column: the columns right of it show its children
        function select(column, index) {
            const nodes = columnsModel.get(column).nodes
            if (index < 0 || index >= nodes.length) {
                return
            }

            while (columnsModel.count > column + 1) {
                columnsModel.remove(columnsModel.count - 1)
            }

            columnsModel.setProperty(column, "selected", index)

            const children = nodes[index].children
            if (children && children.length > 0) {
                columnsModel.append({ nodes: children, selected: -1 })
            }

            Qt.callLater(function() {
                columnsFlickable.contentX = Math.max(0, columnsFlickable.contentWidth - columnsFlickable.width)
            })
        }

        function selectTarget(index) {
            prv.selectedTarget = index
            prv.rebuildColumns()
        }

        function choose(column, index) {
            const node = columnsModel.get(column).nodes[index]
            prv.select(column, index)

            if (node.key !== "" && prv.target) {
                root.mixerModel.setInstrument(prv.target.kind, prv.target.index, node.key)
            }
        }
    }

    //! The favorites change the first column while the dialog is open
    Connections {
        target: root.mixerModel

        function onFavoriteInstrumentsChanged() {
            if (root.visible && columnsModel.count > 0) {
                prv.rebuildColumns()
            }
        }

        function onInstrumentCatalogChanged() {
            if (root.visible) {
                prv.rebuildColumns()
            }
        }
    }

    ListModel {
        id: columnsModel

        dynamicRoles: true
    }

    NavigationPanel {
        id: navPanel

        name: "PlaybackInstrumentsDialog"
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

        width: Math.min(1000, root.width - 32)
        height: Math.min(640, root.height - 32)

        radius: 8
        color: ui.theme.backgroundPrimaryColor
        border.width: ui.theme.borderWidth
        border.color: ui.theme.strokeColor

        //! Keeps clicks inside the dialog from closing it
        MouseArea {
            anchors.fill: parent
        }

        StyledTextLabel {
            id: titleLabel

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: closeButton.left
            anchors.margins: prv.padding

            horizontalAlignment: Text.AlignLeft
            font: ui.theme.largeBodyBoldFont
            text: qsTrc("appshell", "Playback instruments")
        }

        FlatButton {
            id: closeButton

            anchors.right: parent.right
            anchors.rightMargin: prv.padding
            anchors.verticalCenter: titleLabel.verticalCenter

            height: prv.touchSize
            minWidth: 132

            text: qsTrc("appshell", "Done")
            accentButton: true

            navigation.name: "DoneButton"
            navigation.panel: navPanel
            navigation.order: 1000

            onClicked: {
                root.close()
            }
        }

        //! The voices and fader groups
        Item {
            id: targetsPane

            anchors.top: titleLabel.bottom
            anchors.topMargin: prv.padding
            anchors.bottom: parent.bottom
            anchors.bottomMargin: prv.padding
            anchors.left: parent.left
            anchors.leftMargin: prv.padding

            width: Math.min(300, dialog.width * 0.32)

            StyledListView {
                id: targetsList

                anchors.fill: parent

                model: prv.targets
                spacing: 2

                delegate: Rectangle {
                    id: targetItem

                    required property var modelData
                    required property int index

                    readonly property bool isSelected: prv.selectedTarget === targetItem.index
                    readonly property bool isGroup: targetItem.modelData.kind === "group"

                    width: targetsList.width - 12
                    height: prv.touchSize + 4

                    radius: 6
                    color: targetItem.isSelected ? prv.selectionColor : "transparent"

                    Column {
                        anchors.left: parent.left
                        anchors.leftMargin: 10 + targetItem.modelData.indent * 20
                        anchors.right: parent.right
                        anchors.rightMargin: 8
                        anchors.verticalCenter: parent.verticalCenter

                        StyledTextLabel {
                            width: parent.width
                            horizontalAlignment: Text.AlignLeft
                            font: targetItem.isGroup ? ui.theme.bodyBoldFont : ui.theme.bodyFont
                            color: targetItem.isSelected ? "white" : ui.theme.fontPrimaryColor
                            text: targetItem.modelData.title
                        }

                        StyledTextLabel {
                            width: parent.width
                            horizontalAlignment: Text.AlignLeft
                            opacity: 0.7
                            color: targetItem.isSelected ? "white" : ui.theme.fontPrimaryColor
                            text: targetItem.modelData.instrumentName
                        }
                    }

                    MouseArea {
                        anchors.fill: parent

                        onClicked: {
                            prv.selectTarget(targetItem.index)
                        }
                    }
                }
            }
        }

        SeparatorLine {
            id: paneSeparator

            anchors.left: targetsPane.right
            anchors.leftMargin: prv.padding / 2
            anchors.top: targetsPane.top
            anchors.bottom: targetsPane.bottom

            orientation: Qt.Vertical
        }

        //! The instrument browser: one column per level
        Item {
            anchors.top: targetsPane.top
            anchors.bottom: targetsPane.bottom
            anchors.left: paneSeparator.right
            anchors.leftMargin: prv.padding / 2
            anchors.right: parent.right
            anchors.rightMargin: prv.padding

            StyledTextLabel {
                anchors.centerIn: parent
                width: parent.width - 2 * prv.padding

                visible: !prv.target
                wrapMode: Text.WordWrap
                opacity: 0.7
                text: qsTrc("appshell", "Select a voice or a group on the left, then choose the instrument.")
            }

            StyledTextLabel {
                anchors.centerIn: parent
                width: parent.width - 2 * prv.padding

                visible: !!prv.target && root.mixerModel.instrumentTree.length === 0
                wrapMode: Text.WordWrap
                opacity: 0.7
                text: qsTrc("appshell", "No playback instruments found.")
            }

            Flickable {
                id: columnsFlickable

                anchors.fill: parent

                visible: !!prv.target
                clip: true
                contentWidth: columnsRow.width
                contentHeight: height
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.HorizontalFlick

                Row {
                    id: columnsRow

                    height: columnsFlickable.height

                    Repeater {
                        model: columnsModel

                        delegate: Item {
                            id: column

                            required property int index
                            required property var nodes
                            required property int selected

                            width: prv.columnWidth
                            height: columnsRow.height

                            StyledListView {
                                id: nodesList

                                anchors.fill: parent
                                anchors.rightMargin: 1

                                model: column.nodes

                                delegate: Rectangle {
                                    id: nodeItem

                                    required property var modelData
                                    required property int index

                                    readonly property bool isInstrument: nodeItem.modelData.key !== ""
                                    readonly property bool isSelected: column.selected === nodeItem.index
                                    readonly property bool isCurrent: nodeItem.isInstrument && !!prv.target
                                                                      && prv.target.instrumentKey === nodeItem.modelData.key
                                    readonly property bool isFavorite: nodeItem.isInstrument
                                                                       && prv.favoriteKeys.indexOf(nodeItem.modelData.key) !== -1

                                    width: nodesList.width - 10
                                    height: prv.touchSize

                                    radius: 6
                                    color: nodeItem.isCurrent ? prv.selectionColor
                                                              : nodeItem.isSelected ? Utils.colorWithAlpha(ui.theme.fontPrimaryColor, 0.15)
                                                                                    : "transparent"

                                    readonly property color textColor: nodeItem.isCurrent ? "white" : ui.theme.fontPrimaryColor

                                    MouseArea {
                                        anchors.fill: parent

                                        onClicked: {
                                            prv.choose(column.index, nodeItem.index)
                                        }
                                    }

                                    Column {
                                        anchors.left: parent.left
                                        anchors.leftMargin: 10
                                        anchors.right: starLabel.visible ? starLabel.left : chevronLabel.left
                                        anchors.rightMargin: 4
                                        anchors.verticalCenter: parent.verticalCenter

                                        StyledTextLabel {
                                            width: parent.width
                                            horizontalAlignment: Text.AlignLeft
                                            color: nodeItem.textColor
                                            text: nodeItem.modelData.title
                                        }

                                        StyledTextLabel {
                                            width: parent.width
                                            visible: text !== ""
                                            horizontalAlignment: Text.AlignLeft
                                            opacity: 0.7
                                            color: nodeItem.textColor
                                            text: nodeItem.modelData.subtitle
                                        }
                                    }

                                    //! Adds the instrument to the favorites or removes it
                                    StyledTextLabel {
                                        id: starLabel

                                        anchors.right: chevronLabel.left
                                        anchors.verticalCenter: parent.verticalCenter

                                        visible: nodeItem.isInstrument
                                        width: prv.touchSize
                                        height: prv.touchSize
                                        font: Qt.font({ family: ui.theme.bodyFont.family, pixelSize: 22 })
                                        color: nodeItem.isFavorite ? "#E0B23A" : nodeItem.textColor
                                        opacity: nodeItem.isFavorite ? 1.0 : 0.5
                                        text: nodeItem.isFavorite ? "★" : "☆"

                                        MouseArea {
                                            anchors.fill: parent

                                            onClicked: {
                                                root.mixerModel.toggleFavoriteInstrument(nodeItem.modelData.key)
                                            }
                                        }
                                    }

                                    StyledTextLabel {
                                        id: chevronLabel

                                        anchors.right: parent.right
                                        anchors.rightMargin: 8
                                        anchors.verticalCenter: parent.verticalCenter

                                        width: nodeItem.isInstrument ? 0 : 16
                                        color: nodeItem.textColor
                                        text: nodeItem.isInstrument ? "" : "›"
                                    }
                                }
                            }

                            Rectangle {
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom

                                width: 1
                                color: ui.theme.strokeColor
                            }
                        }
                    }
                }
            }
        }
    }
}
