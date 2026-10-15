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

#include "settings.h"

namespace mu::appshell {
//! Largest size of a staff in the presentation window, in percent of the window height.
//! A staff (with its share of lyrics etc.) is the height of the system divided by the number of staves.
//! The default is what a score with five staves (a choir score) gets when it fills the window:
//! 1 / (5 * (1 + 2 * vertical margin of the view)). Scores with fewer staves are not enlarged beyond it.
constexpr double MAX_STAFF_HEIGHT_PERCENT_MIN = 5.0;
constexpr double MAX_STAFF_HEIGHT_PERCENT_MAX = 100.0;
constexpr double MAX_STAFF_HEIGHT_PERCENT_DEFAULT = 17.9;

inline const muse::Settings::Key& maxStaffHeightKey()
{
    static const muse::Settings::Key key("appshell", "presentation/maxStaffHeightPercent");
    return key;
}

inline double maxStaffHeightPercent()
{
    muse::Settings* s = muse::settings();
    s->setDefaultValue(maxStaffHeightKey(), muse::Val(MAX_STAFF_HEIGHT_PERCENT_DEFAULT));
    return s->value(maxStaffHeightKey()).toDouble();
}
}
