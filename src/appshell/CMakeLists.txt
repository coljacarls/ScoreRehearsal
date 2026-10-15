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

# The ScoreRehearsal application shell. It replaces MuseScore's "appshell" module
# (same target name, same module name, same QML module URI), so that musescore/src/app
# links it without changes.

muse_create_module(appshell)

target_sources(appshell PRIVATE
    appshellmodule.cpp
    appshellmodule.h
    appshellcommands.h
    iappshellconfiguration.h

    internal/appshellcommandscontroller.cpp
    internal/appshellcommandscontroller.h
    internal/appshellcommandsregister.cpp
    internal/appshellcommandsregister.h
    internal/appshellcommandsstate.cpp
    internal/appshellcommandsstate.h
    internal/appshellconfiguration.cpp
    internal/appshellconfiguration.h
    internal/istartupscenario.h
    internal/startupscenario.cpp
    internal/startupscenario.h

    widgets/splashscreen/splashscreen.h
)

# The version changes with every commit (git describe), so only this file gets it
set_source_files_properties(internal/appshellconfiguration.cpp PROPERTIES
    COMPILE_DEFINITIONS "SCOREREHEARSAL_VERSION=\"${SCOREREHEARSAL_VERSION}\""
    SKIP_UNITY_BUILD_INCLUSION ON
)

# musescore/src/app/main.cpp expects this resource (QT_OPENGL_BUGLIST)
qt_add_resources(appshell appshell_resources
    PREFIX
        /
    FILES
        resources/win_opengl_buglist.json
)

target_link_libraries(appshell PRIVATE
    muse::ui
    muse::uicomponents
    Qt::Quick
    Qt::Widgets
)

if (MUE_BUILD_APPSHELL_QML)
    add_subdirectory(qml/MuseScore/AppShell)
endif()
