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

#include "appshellconfiguration.h"

#include "settings.h"

using namespace mu::appshell;
using namespace muse;

#ifndef SCOREREHEARSAL_VERSION
#define SCOREREHEARSAL_VERSION "0.0.0"
#endif

static const std::string SOURCE_CODE_URL("https://github.com/coljacarls/ScoreRehearsal");

void AppShellConfiguration::init()
{
}

std::string AppShellConfiguration::appVersion() const
{
    return SCOREREHEARSAL_VERSION;
}

std::string AppShellConfiguration::museScoreVersion() const
{
    return String(application()->version().toString() + u"." + application()->build()).toStdString();
}

std::string AppShellConfiguration::museScoreRevision() const
{
    return application()->revision().toStdString();
}

std::string AppShellConfiguration::sourceCodeUrl() const
{
    return SOURCE_CODE_URL;
}

void AppShellConfiguration::revertToFactorySettings(bool keepDefaultSettings, bool notifyAboutChanges,
                                                    bool notifyOtherInstances) const
{
    settings()->reset(keepDefaultSettings, notifyAboutChanges, notifyOtherInstances);
}
