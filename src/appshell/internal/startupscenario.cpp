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

#include "startupscenario.h"

#include "actions/actiontypes.h"
#include "project/types/projecturis.h"

#include "log.h"

using namespace mu::appshell;
using namespace muse;
using namespace muse::actions;

void StartupScenario::setStartupType(const std::optional<std::string>&)
{
    // The only startup types are "empty" and "with the given file"
}

bool StartupScenario::isStartWithNewFileAsSecondaryInstance() const
{
    return false;
}

const mu::project::ProjectFile& StartupScenario::startupScoreFile() const
{
    return m_startupScoreFile;
}

void StartupScenario::setStartupScoreFile(const std::optional<project::ProjectFile>& file)
{
    m_startupScoreFile = file ? file.value() : project::ProjectFile();
}

void StartupScenario::runOnSplashScreen()
{
}

void StartupScenario::runAfterSplashScreen()
{
    TRACEFUNC;

    if (m_startupCompleted) {
        return;
    }

    m_startupCompleted = true;

    interactive()->open(project::NOTATION_REHEARSAL_PAGE_URI).onResolve(this, [this](const Val&) {
        if (m_startupScoreFile.isValid()) {
            openScore(m_startupScoreFile);
        }
    });
}

bool StartupScenario::startupCompleted() const
{
    return m_startupCompleted;
}

void StartupScenario::openScore(const project::ProjectFile& file)
{
    //! NOTE "file-open" is the project module's action, mapped to command://project/open
    dispatcher()->dispatch("file-open", ActionData::make_arg2<QUrl, QString>(file.url, file.displayNameOverride));
}
