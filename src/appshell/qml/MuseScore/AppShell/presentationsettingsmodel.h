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

#include <QObject>
#include <qqmlintegration.h>

#include "async/asyncable.h"

namespace mu::appshell {
//! Settings of the presentation window. Changes apply (and persist) immediately.
class PresentationSettingsModel : public QObject, public muse::async::Asyncable
{
    Q_OBJECT

    Q_PROPERTY(double maxStaffHeightPercent READ maxStaffHeightPercent WRITE setMaxStaffHeightPercent
               NOTIFY maxStaffHeightPercentChanged)
    Q_PROPERTY(double minPercent READ minPercent CONSTANT)
    Q_PROPERTY(double maxPercent READ maxPercent CONSTANT)

    QML_ELEMENT

public:
    explicit PresentationSettingsModel(QObject* parent = nullptr);

    double maxStaffHeightPercent() const;
    void setMaxStaffHeightPercent(double percent);

    double minPercent() const;
    double maxPercent() const;

    Q_INVOKABLE void resetMaxStaffHeight();

signals:
    void maxStaffHeightPercentChanged();
};
}
