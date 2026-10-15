/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited and others
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

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Window

import Muse.Ui
import Muse.UiComponents
import MuseScore.AppShell
import MuseScore.Playback

//! Touch-oriented, read-only rehearsal page: the whole window content of ScoreRehearsal.
//! Adapted from MuseScore's RehearsalPage, without the dock framework.
Item {
    id: root

    property RehearsalPageModel pageModel: RehearsalPageModel {}

    //! Immersive mode: OS fullscreen, no top bar; the controls are shown in a floating bar over the score
    readonly property bool immersive: prv.immersive

    Component.onCompleted: {
        root.pageModel.load()
    }

    function setImmersive(immersive) {
        if (immersive === prv.immersive) {
            return
        }

        if (immersive) {
            prv.fullScreenSetByImmersive = !root.pageModel.isFullScreen
            prv.immersive = true
            root.pageModel.setFullScreen(true)
        } else {
            prv.immersive = false
            if (prv.fullScreenSetByImmersive) {
                prv.fullScreenSetByImmersive = false
                root.pageModel.setFullScreen(false)
            }
        }
    }

    QtObject {
        id: prv

        property bool immersive: false
        property bool fullScreenSetByImmersive: false

        //! Where the presentation window was the last time (e.g. on the second screen)
        property rect presentationGeometry: Qt.rect(100, 100, 1280, 360)
    }

    Connections {
        target: root.pageModel

        function onIsFullScreenChanged() {
            //! Fullscreen was left by other means (F11, ...)
            if (!root.pageModel.isFullScreen && prv.immersive) {
                prv.fullScreenSetByImmersive = false
                root.setImmersive(false)
            }
        }

        function onHasProjectChanged() {
            if (!root.pageModel.hasProject) {
                root.setImmersive(false)
            }
        }
    }

    NavigationSection {
        id: topBarNavSec
        name: "RehearsalTopBar"
        enabled: root.visible && !root.immersive
        order: 1
    }

    NavigationSection {
        id: immersiveControlsNavSec
        name: "RehearsalImmersiveControls"
        enabled: root.visible && root.immersive
        order: 2
    }

    NavigationSection {
        id: tracksPanelNavSec
        name: "RehearsalTracksPanel"
        enabled: root.visible && root.pageModel.isTracksPanelVisible
        order: 6
    }

    //! Top bar: back to the start page / title on the left, the rehearsal controls on the right
    Rectangle {
        id: topBar

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        height: 48
        visible: !root.immersive

        color: ui.theme.backgroundSecondaryColor

        NavigationPanel {
            id: topBarLeftNavPanel
            name: "RehearsalTopBarLeft"
            enabled: topBar.enabled && topBar.visible
            section: topBarNavSec
            order: 1
            direction: NavigationPanel.Horizontal
            accessible.name: qsTrc("appshell", "Score")
        }

        Row {
            id: leftRow

            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter

            spacing: 8

            FlatButton {
                id: homeButton

                anchors.verticalCenter: parent.verticalCenter
                visible: root.pageModel.hasProject
                height: 36

                icon: IconCode.ARROW_LEFT
                text: qsTrc("appshell", "Home")
                orientation: Qt.Horizontal

                toolTipTitle: qsTrc("appshell", "Back to the start page")

                navigation.panel: topBarLeftNavPanel
                navigation.order: 1

                onClicked: {
                    root.pageModel.closeScore()
                }
            }

            StyledTextLabel {
                anchors.verticalCenter: parent.verticalCenter
                width: Math.min(implicitWidth, topBar.width - leftRow.x - (homeButton.visible ? homeButton.width : 0)
                                - presentationButton.width - partDropdown.width - controls.width - menuButton.width - 96)
                text: root.pageModel.scoreTitle
                font: ui.theme.largeBodyBoldFont
                horizontalAlignment: Text.AlignLeft
                elide: Text.ElideRight
            }

            FlatButton {
                id: presentationButton

                anchors.verticalCenter: parent.verticalCenter
                visible: root.pageModel.hasProject
                width: 36
                height: 36

                icon: IconCode.OPEN_LINK
                accentButton: root.pageModel.isPresentationWindowOpen
                transparent: !root.pageModel.isPresentationWindowOpen

                toolTipTitle: root.pageModel.isPresentationWindowOpen ? qsTrc("appshell", "Close presentation window")
                                                                      : qsTrc("appshell", "Presentation window")
                accessible.checked: root.pageModel.isPresentationWindowOpen

                navigation.panel: topBarLeftNavPanel
                navigation.order: 2

                onClicked: {
                    root.pageModel.isPresentationWindowOpen = !root.pageModel.isPresentationWindowOpen
                }
            }
        }

        //! The full score or one of the parts defined in the score
        StyledDropdown {
            id: partDropdown

            anchors.right: controls.left
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: 200
            height: 36

            visible: root.pageModel.hasProject

            model: {
                const items = [{ text: qsTrc("appshell", "Full score"), value: -1 }]
                const names = root.pageModel.partNames
                for (let i = 0; i < names.length; ++i) {
                    items.push({ text: names[i], value: i })
                }
                return items
            }

            currentIndex: root.pageModel.currentPartIndex + 1

            navigation.panel: topBarLeftNavPanel
            navigation.order: 3
            navigation.accessible.name: qsTrc("appshell", "Part")

            onActivated: function(index, value) {
                root.pageModel.selectPart(value)
            }
        }

        RehearsalControls {
            id: controls

            anchors.right: menuButton.left
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter

            pageModel: root.pageModel

            visible: root.pageModel.hasProject
            immersive: root.immersive
            sidePadding: 0

            navigationSection: topBarNavSec
            navigationOrderStart: 2
            navigationName: "RehearsalToolBarControls"

            onToggleImmersiveRequested: {
                root.setImmersive(!root.immersive)
            }
        }

        FlatButton {
            id: menuButton

            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: 36
            height: 36

            icon: IconCode.MENU_THREE_DOTS
            transparent: !appMenu.isMenuOpened
            accentButton: appMenu.isMenuOpened

            toolTipTitle: qsTrc("appshell", "Menu")

            navigation.panel: topBarLeftNavPanel
            navigation.order: 4

            onClicked: {
                appMenu.toggleOpened([
                    { id: "close", title: qsTrc("appshell", "Close score"), enabled: root.pageModel.hasProject },
                    { id: "fullscreen", title: qsTrc("appshell", "Full screen"), checkable: true, checked: root.pageModel.isFullScreen },
                    { id: "darktheme", title: qsTrc("appshell", "Dark theme"), checkable: true, checked: root.pageModel.isDarkTheme },
                    { },
                    { id: "settings", title: qsTrc("appshell", "Settings…") },
                    { id: "about", title: qsTrc("appshell", "About ScoreRehearsal…") },
                    { },
                    { id: "quit", title: qsTrc("appshell", "Quit") }
                ])
            }

            StyledMenuLoader {
                id: appMenu

                menuAnchorItem: root.Window.window ? root.Window.window.contentItem : null

                onHandleMenuItem: function(itemId) {
                    switch (itemId) {
                    case "close": root.pageModel.closeScore(); break
                    case "fullscreen": root.pageModel.setFullScreen(!root.pageModel.isFullScreen); break
                    case "darktheme": root.pageModel.setDarkTheme(!root.pageModel.isDarkTheme); break
                    case "settings": audioSettingsDialog.open(); break
                    case "about": root.pageModel.showAbout(); break
                    case "quit": root.pageModel.quit(); break
                    }
                }
            }
        }
    }

    SeparatorLine {
        id: topBarSeparator
        anchors.top: topBar.bottom
        visible: topBar.visible
    }

    //! Page content
    Item {
        id: content

        anchors.top: topBar.visible ? topBarSeparator.bottom : parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        OpenScorePlaceholder {
            anchors.fill: parent

            visible: !root.pageModel.hasProject

            onOpenRequested: {
                root.pageModel.openScore()
            }

            onOpenSessionRequested: function(sessionId) {
                root.pageModel.openSession(sessionId)
            }

            onRemoveSessionRequested: function(sessionId) {
                root.pageModel.removeSession(sessionId)
            }
        }

        RehearsalNotationView {
            id: notationView

            name: "RehearsalNotationView"

            visible: root.pageModel.hasProject

            viewModeOverride: root.pageModel.viewModeOverride

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: tracksPanel.visible ? tracksPanel.left : parent.right
            anchors.bottom: parent.bottom
        }

        RehearsalMixerPanel {
            id: tracksPanel

            anchors.top: parent.top
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            width: 340

            visible: root.pageModel.hasProject && root.pageModel.isTracksPanelVisible

            navigationSection: tracksPanelNavSec
            navigationOrderStart: 1

            onCreateGroupRequested: {
                faderGroupDialog.open()
            }
        }

        //! Floating control bar of the immersive mode
        Rectangle {
            id: floatingBar

            anchors.horizontalCenter: notationView.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 16

            width: floatingControls.implicitWidth
            height: floatingControls.implicitHeight + 12
            radius: height / 2

            visible: root.immersive

            color: Utils.colorWithAlpha(ui.theme.backgroundPrimaryColor, 0.9)
            border.color: ui.theme.strokeColor
            border.width: ui.theme.borderWidth

            //! Do not let clicks on the bar's background reach the score
            MouseArea {
                anchors.fill: parent
            }

            RehearsalControls {
                id: floatingControls

                anchors.centerIn: parent

                pageModel: root.pageModel

                immersive: root.immersive
                presentationButtonVisible: true
                buttonSize: 40
                sidePadding: 12

                navigationSection: immersiveControlsNavSec
                navigationOrderStart: 1
                navigationName: "RehearsalImmersiveControls"

                onToggleImmersiveRequested: {
                    root.setImmersive(!root.immersive)
                }
            }
        }
    }

    //! The window for the audience; the model keeps the session-wide "open" state,
    //! so the button in either control bar (top bar, floating bar) opens and closes it
    Loader {
        id: presentationWindowLoader

        active: root.pageModel.hasProject && root.pageModel.isPresentationWindowOpen

        sourceComponent: PresentationWindow {
            initialGeometry: prv.presentationGeometry
            soloPartIds: tracksPanel.mixerModel.soloPartIds

            onCloseRequested: function(geometry) {
                prv.presentationGeometry = geometry
                root.pageModel.isPresentationWindowOpen = false
            }
        }
    }

    NavigationSection {
        id: faderGroupDialogNavSec
        name: "FaderGroupDialog"
        enabled: faderGroupDialog.visible
        order: 7
    }

    //! Modal dialog to create a fader group, over the whole page
    FaderGroupDialog {
        id: faderGroupDialog

        anchors.fill: parent
        z: 100

        mixerModel: tracksPanel.mixerModel
        navigationSection: faderGroupDialogNavSec
    }

    NavigationSection {
        id: audioSettingsNavSec
        name: "AudioSettingsDialog"
        enabled: audioSettingsDialog.visible
        order: 8
    }

    //! Modal settings dialog (audio driver and buffer size), over the whole page
    AudioSettingsDialog {
        id: audioSettingsDialog

        anchors.fill: parent
        z: 100

        navigationSection: audioSettingsNavSec
    }

    //! Drag & drop of score files anywhere on the page
    DropArea {
        anchors.fill: parent

        onEntered: function(drag) {
            drag.accepted = root.droppedScoreUrl(drag.urls) !== null
        }

        onDropped: function(drop) {
            const url = root.droppedScoreUrl(drop.urls)
            if (url !== null) {
                drop.accept()
                root.pageModel.openScoreUrl(url)
            }
        }
    }

    function droppedScoreUrl(urls) {
        for (let i = 0; i < urls.length; ++i) {
            if (root.pageModel.isUrlSupported(urls[i])) {
                return urls[i]
            }
        }
        return null
    }
}
