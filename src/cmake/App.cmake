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

# Included from musescore/src/app/CMakeLists.txt (configuration "app-external"):
# executable name, icon and resources of the ScoreRehearsal executable.

set(EXECUTABLE_NAME "ScoreRehearsal")

if (OS_IS_WIN AND CC_IS_MSVC)
    set(WINDOWS_ICONS_RC ${MUE_EXTERNAL_APP_DIR}/branding/scorerehearsal_icons.rc)
endif()

# The app resources (themes, shortcuts, ...) are MuseScore's own configs,
# referenced by absolute path from a generated qrc
set(MUSESCORE_APP_CONFIGS_DIR ${PROJECT_SOURCE_DIR}/src/app/configs)
configure_file(${MUE_EXTERNAL_APP_DIR}/app/app.qrc.in ${CMAKE_BINARY_DIR}/external_app/app.qrc @ONLY)
set(APP_QRC ${CMAKE_BINARY_DIR}/external_app/app.qrc)
