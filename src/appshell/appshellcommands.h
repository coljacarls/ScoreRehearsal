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

#pragma once

#include "rcommand/commandtypes.h"
#include "types/uri.h"

namespace mu::appshell {
//! The command ids match MuseScore's, so that the default shortcuts
//! (Ctrl+Q, F11) from musescore/src/app/configs/data/shortcuts.json apply
inline static const muse::rcommand::Command APP_QUIT_COMMAND("command://app/quit");
inline static const muse::rcommand::Command APP_FULLSCREEN_COMMAND("command://app/fullscreen");
inline static const muse::rcommand::Command APP_ABOUT_COMMAND("command://app/about-scorerehearsal");

inline static const muse::Uri ABOUT_URI("scorerehearsal://about");
}
