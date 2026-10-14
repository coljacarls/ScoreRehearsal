# SPDX-License-Identifier: GPL-3.0-only
#
# ScoreRehearsal
# Copyright (C) 2026 Colja Carls
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# The QML module keeps MuseScore's URI "MuseScore.AppShell":
# musescore/src/app/internal/guiapp.cpp loads the main window from
# ":/qt/qml/MuseScore/AppShell/platform/<os>/Main.qml".

muse_create_qml_module(appshell_qml FOR appshell)

qt_add_qml_module(appshell_qml
    URI MuseScore.AppShell
    VERSION 1.0
    SOURCES
        aboutmodel.cpp
        aboutmodel.h
        mainwindowtitleprovider.cpp
        mainwindowtitleprovider.h
        rehearsalpagemodel.cpp
        rehearsalpagemodel.h
    QML_FILES
        AboutDialog.qml
        AppWindow.qml
        OpenScorePlaceholder.qml
        RehearsalControls.qml
        RehearsalNotationView.qml
        RehearsalPage.qml
    RESOURCES
        resources/scorerehearsal_logo.png
    IMPORTS
        TARGET muse_ui_qml
        TARGET muse_uicomponents_qml
        TARGET muse_interactive_qml
        TARGET muse_shortcuts_qml
        TARGET notationscene_qml
        TARGET playback_qml
)

if (OS_IS_MAC)
    qt_target_qml_sources(appshell_qml
        QML_FILES
            platform/mac/Main.qml
    )
elseif(OS_IS_WIN)
    qt_target_qml_sources(appshell_qml
        QML_FILES
            platform/win/AppSystemButtons.qml
            platform/win/AppTitleBar.qml
            platform/win/Main.qml
    )
elseif(OS_IS_LIN OR OS_IS_FREEBSD)
    qt_target_qml_sources(appshell_qml
        QML_FILES
            platform/linux/Main.qml
    )
else()
    message(FATAL_ERROR "Unsupported OS")
endif()

fixup_qml_module_dependencies(appshell_qml)
