# SPDX-License-Identifier: GPL-3.0-only
#
# ScoreRehearsal
# A lightweight, read-only rehearsal player built on MuseScore Studio components
#
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

###########################################
# ScoreRehearsal: module switches and branding
###########################################
#
# Included from musescore/SetupConfigure.cmake (configuration "app-external",
# MUE_EXTERNAL_APP_DIR = this directory's parent).
#
# Upstream merge checklist: compare this file with the APP-WEB block in
# musescore/SetupConfigure.cmake and with the option lists in musescore/CMakeLists.txt
# (MUE_BUILD_*) and musescore/muse/framework/cmake/MuseDeclareOptions.cmake (MUSE_MODULE_*).
# Every new module option should be switched OFF here explicitly unless the app needs it.

set(SCOREREHEARSAL_VERSION "0.1.0")
add_compile_definitions(
    SCOREREHEARSAL_VERSION="${SCOREREHEARSAL_VERSION}"
    MUE_APP_ORGANIZATION_NAME="ScoreRehearsal"
    MUE_APP_ORGANIZATION_DOMAIN="scorerehearsal.carls.de"
)

# --- Branding: no MuseScore name, logo or identifiers ---
set(MUSE_APP_NAME_HUMAN_READABLE "ScoreRehearsal")
set(MUSE_APP_NAME_MACHINE_READABLE "ScoreRehearsal")
set(MUSE_APP_GUI_IDENTIFIER "de.carls.ScoreRehearsal")
set(MUSE_APP_ICON_WIN "${MUE_EXTERNAL_APP_DIR}/branding/ScoreRehearsal.ico")

# --- App ---
set(MUE_ENABLE_CONSOLEAPP ON)     # needed: the command line parser provides the startup score file
set(MUE_ENABLE_SPLASHSCREEN OFF)
set(MUSE_ENABLE_UNIT_TESTS OFF)
set(MUE_RUN_LRELEASE OFF)         # no translations (languages module is off)
set(MUE_INSTALL_SOUNDFONT ON)     # MS Basic (MIT licensed) is the only sound source

# --- Framework modules: ON ---
set(MUSE_MODULE_ACCESSIBILITY ON)
set(MUSE_MODULE_ACTIONS ON)
set(MUSE_MODULE_AUDIO ON)
set(MUSE_MODULE_AUDIO_EXPORT OFF)
set(MUSE_MODULE_DIAGNOSTICS ON)
set(MUSE_MODULE_DIAGNOSTICS_CRASHPAD_CLIENT OFF)
set(MUSE_MODULE_DRAW ON)
set(MUSE_MODULE_GLOBAL ON)
set(MUSE_MODULE_INTERACTIVE ON)
set(MUSE_MODULE_MPE ON)
set(MUSE_MODULE_RCOMMAND ON)
set(MUSE_MODULE_RCONTROL ON)
set(MUSE_MODULE_SHORTCUTS ON)
set(MUSE_MODULE_UI ON)

# --- Framework modules: OFF (stubs provide the interfaces other modules need) ---
set(MUSE_MODULE_AUDIOPLUGINS OFF)
set(MUSE_MODULE_AUTOMATION OFF)
set(MUSE_MODULE_CLOUD OFF)
set(MUSE_MODULE_DOCKWINDOW OFF)   # KDDockWidgets is not needed: the shell has a fixed layout
set(MUSE_MODULE_EXTENSIONS OFF)
set(MUSE_MODULE_LANGUAGES OFF)
set(MUSE_MODULE_LEARN OFF)
set(MUSE_MODULE_MIDI OFF)
set(MUSE_MODULE_MIDIREMOTE OFF)
set(MUSE_MODULE_MULTIWINDOWS OFF)
set(MUSE_MODULE_MUSESAMPLER OFF)  # proprietary sampler, never loaded
set(MUSE_MODULE_NETWORK OFF)      # the app never contacts a server
set(MUSE_MODULE_TESTFLOW OFF)
set(MUSE_MODULE_TOAST OFF)
set(MUSE_MODULE_TOURS OFF)
set(MUSE_MODULE_UPDATE OFF)
set(MUSE_MODULE_VST OFF)
set(MUSE_MODULE_WORKSPACE OFF)

# --- MuseScore modules: ON ---
set(MUE_BUILD_APPSHELL_MODULE ON)         # -> ${MUE_EXTERNAL_APP_DIR}/appshell (see musescore/src/CMakeLists.txt)
set(MUE_BUILD_APPSHELL_QML ON)
set(MUE_BUILD_NOTATION_MODULE ON)
set(MUE_BUILD_NOTATIONSCENE_MODULE ON)
set(MUE_BUILD_PLAYBACK_MODULE ON)
set(MUE_BUILD_PROJECT_MODULE ON)
set(MUE_BUILD_ENGRAVING_QML ON)
set(MUE_BUILD_ENGRAVING_PLAYBACK ON)
set(MUE_BUILD_ENGRAVING_DEVTOOLS ON)   # OFF does not compile upstream (engravingmodule.cpp registers the devtools-only EngravingElementsProvider)
# NotationView.qml (element popups) imports these two QML modules, so they stay on
set(MUE_BUILD_INSTRUMENTSSCENE_MODULE ON)
set(MUE_BUILD_PROPERTIESPANEL_MODULE ON)

# --- MuseScore modules: OFF ---
set(MUE_BUILD_BRAILLE_MODULE OFF)
set(MUE_BUILD_BRAILLE_QML ON)             # the stub's QML module (NotationView.qml imports MuseScore.Braille)
set(MUE_BUILD_CONVERTER_MODULE OFF)
set(MUE_BUILD_MUSESOUNDS_MODULE OFF)      # MuseSounds / Muse Hub promotion and update checks
set(MUE_BUILD_PALETTE_MODULE OFF)
set(MUE_BUILD_PALETTE_QML OFF)
set(MUE_BUILD_PREFERENCES_MODULE OFF)
set(MUE_BUILD_PRINT_MODULE OFF)
set(MUE_BUILD_MACOS_INTEGRATION OFF)

# --- Import / export: none (only .mscz / .mscx, which the engraving module reads itself) ---
set(MUE_BUILD_IMPEXP_BB_MODULE OFF)
set(MUE_BUILD_IMPEXP_BWW_MODULE OFF)
set(MUE_BUILD_IMPEXP_CAPELLA_MODULE OFF)
set(MUE_BUILD_IMPEXP_MIDI_MODULE OFF)
set(MUE_BUILD_IMPEXP_MLSCORE_MODULE OFF)
set(MUE_BUILD_IMPEXP_MNX_MODULE OFF)
set(MUE_BUILD_IMPEXP_MUSEDATA_MODULE OFF)
set(MUE_BUILD_IMPEXP_MUSICXML_MODULE OFF)
set(MUE_BUILD_IMPEXP_OVE_MODULE OFF)
set(MUE_BUILD_IMPEXP_AUDIOEXPORT_MODULE OFF)
set(MUE_BUILD_IMPEXP_IMAGESEXPORT_MODULE OFF)
set(MUE_BUILD_IMPEXP_GUITARPRO_MODULE OFF)
set(MUE_BUILD_IMPEXP_MEI_MODULE OFF)
set(MUE_BUILD_IMPEXP_VIDEOEXPORT_MODULE OFF)
set(MUE_BUILD_IMPEXP_TABLEDIT_MODULE OFF)
set(MUE_BUILD_IMPEXP_LYRICS_MODULE OFF)
set(MUE_BUILD_IMPORTEXPORT_TESTS OFF)
