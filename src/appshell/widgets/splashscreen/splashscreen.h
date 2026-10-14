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

namespace mu::appshell {
//! Compatibility declaration for musescore/src/app/internal/guiapp.h, which includes
//! "appshell/widgets/splashscreen/splashscreen.h" unconditionally and uses the enum
//! in its SplashConfig. ScoreRehearsal has no splash screen (MUE_ENABLE_SPLASHSCREEN is off),
//! so the class is never instantiated.
class SplashScreen
{
public:
    enum SplashScreenType {
        Default,
        ForNewInstance
    };
};
}
