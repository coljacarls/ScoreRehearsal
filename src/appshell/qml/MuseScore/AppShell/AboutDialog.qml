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
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import MuseScore.AppShell

StyledDialogView {
    id: root

    title: qsTrc("appshell", "About ScoreRehearsal")

    contentHeight: 460
    contentWidth: 520

    AboutModel {
        id: aboutModel
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 24

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 32
            Layout.leftMargin: 40
            Layout.rightMargin: 40

            spacing: 20

            Image {
                Layout.alignment: Qt.AlignHCenter

                source: "resources/scorerehearsal_logo.png"
                sourceSize: Qt.size(80, 80)
                width: 80
                height: 80
            }

            Column {
                spacing: 6
                Layout.fillWidth: true

                StyledTextLabel {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "ScoreRehearsal " + aboutModel.appVersion()
                    font: ui.theme.headerBoldFont
                }

                StyledTextLabel {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTrc("appshell", "A read-only rehearsal player for MuseScore files")
                }
            }

            StyledTextLabel {
                Layout.fillWidth: true
                text: qsTrc("appshell", "Based on MuseScore Studio %1 (revision %2), free software published by MuseScore Limited and others under the GNU General Public License version 3. ScoreRehearsal is not affiliated with or endorsed by MuseScore.")
                      .arg(aboutModel.museScoreVersion())
                      .arg(aboutModel.museScoreRevision())
                wrapMode: Text.WordWrap
                maximumLineCount: 6
                horizontalAlignment: Text.AlignHCenter
            }

            StyledTextLabel {
                Layout.fillWidth: true
                text: qsTrc("appshell", "Copyright © 2026 Colja Carls. Licensed under the GNU General Public License version 3. Source code: <a href=\"%1\">%1</a>")
                      .arg(aboutModel.sourceCodeUrl())
                wrapMode: Text.WordWrap
                maximumLineCount: 4
                horizontalAlignment: Text.AlignHCenter
            }

            StyledTextLabel {
                Layout.fillWidth: true
                text: qsTrc("appshell", "Sounds: MS Basic (MIT license). Music fonts: Leland, Bravura, Edwin, Petaluma and others (SIL Open Font License). See THIRD_PARTY_NOTICES.md for details.")
                wrapMode: Text.WordWrap
                maximumLineCount: 4
                horizontalAlignment: Text.AlignHCenter
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.bottomMargin: 20
            Layout.leftMargin: 40
            Layout.rightMargin: 40

            FlatButton {
                text: qsTrc("appshell", "Copy revision")

                onClicked: {
                    aboutModel.copyRevisionToClipboard()
                }
            }

            Item {
                Layout.fillWidth: true
            }

            FlatButton {
                text: qsTrc("appshell", "OK")
                accentButton: true

                onClicked: {
                    root.hide()
                }
            }
        }
    }
}
