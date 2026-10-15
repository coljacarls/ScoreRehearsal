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

#include "presentationsettingsmodel.h"

#include <algorithm>

#include "presentationsettings.h"

using namespace mu::appshell;

PresentationSettingsModel::PresentationSettingsModel(QObject* parent)
    : QObject(parent)
{
    muse::settings()->valueChanged(maxStaffHeightKey()).onReceive(this, [this](const muse::Val&) {
        emit maxStaffHeightPercentChanged();
    });
}

double PresentationSettingsModel::maxStaffHeightPercent() const
{
    return std::clamp(mu::appshell::maxStaffHeightPercent(), MAX_STAFF_HEIGHT_PERCENT_MIN, MAX_STAFF_HEIGHT_PERCENT_MAX);
}

void PresentationSettingsModel::setMaxStaffHeightPercent(double percent)
{
    percent = std::clamp(percent, MAX_STAFF_HEIGHT_PERCENT_MIN, MAX_STAFF_HEIGHT_PERCENT_MAX);
    muse::settings()->setSharedValue(maxStaffHeightKey(), muse::Val(percent));
}

double PresentationSettingsModel::minPercent() const
{
    return MAX_STAFF_HEIGHT_PERCENT_MIN;
}

double PresentationSettingsModel::maxPercent() const
{
    return MAX_STAFF_HEIGHT_PERCENT_MAX;
}

void PresentationSettingsModel::resetMaxStaffHeight()
{
    setMaxStaffHeightPercent(MAX_STAFF_HEIGHT_PERCENT_DEFAULT);
}
