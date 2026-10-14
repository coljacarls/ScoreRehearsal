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

#include "../iappshellconfiguration.h"

#include "modularity/ioc.h"
#include "global/iapplication.h"

namespace mu::appshell {
class AppShellConfiguration : public IAppShellConfiguration, public muse::Contextable
{
    muse::GlobalInject<muse::IApplication> application;

public:
    AppShellConfiguration(const muse::modularity::ContextPtr& iocCtx)
        : muse::Contextable(iocCtx) {}

    void init();

    std::string appVersion() const override;
    std::string museScoreVersion() const override;
    std::string museScoreRevision() const override;
    std::string sourceCodeUrl() const override;

    void revertToFactorySettings(bool keepDefaultSettings = false, bool notifyAboutChanges = true,
                                 bool notifyOtherInstances = true) const override;
};
}
