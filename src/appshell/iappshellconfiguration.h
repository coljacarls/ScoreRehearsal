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

#include <string>

#include "modularity/imoduleinterface.h"

namespace mu::appshell {
//! Replaces MuseScore's IAppShellConfiguration (musescore/src/appshell/iappshellconfiguration.h).
//! musescore/src/app/internal/guiapp.h includes this header through the external app include path
//! and only needs revertToFactorySettings() from it.
class IAppShellConfiguration : MODULE_GLOBAL_INTERFACE
{
    INTERFACE_ID(IAppshellConfiguration)

public:
    virtual ~IAppShellConfiguration() = default;

    //! ScoreRehearsal's own version
    virtual std::string appVersion() const = 0;

    //! Version and revision of the MuseScore Studio code base this build is based on
    virtual std::string museScoreVersion() const = 0;
    virtual std::string museScoreRevision() const = 0;

    virtual std::string sourceCodeUrl() const = 0;

    virtual void revertToFactorySettings(bool keepDefaultSettings = false, bool notifyAboutChanges = true,
                                         bool notifyOtherInstances = true) const = 0;
};
}
