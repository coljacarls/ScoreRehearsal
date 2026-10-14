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

#include "appshellcommandsstate.h"

#include "../appshellcommands.h"

#include "log.h"

using namespace muse;
using namespace muse::rcommand;
using namespace mu::appshell;

std::string AppshellCommandsState::moduleName() const
{
    return "appshell";
}

void AppshellCommandsState::init()
{
    mainWindow()->isFullScreenChanged().onNotify(this, [this]() {
        m_commandStateChanged.send(APP_FULLSCREEN_COMMAND, commandState(APP_FULLSCREEN_COMMAND));
    });
}

void AppshellCommandsState::deinit()
{
    mainWindow()->isFullScreenChanged().disconnect(this);
}

CommandState AppshellCommandsState::commandState(const Command& command) const
{
    if (command == APP_FULLSCREEN_COMMAND) {
        return CommandState(true, mainWindow()->isFullScreen());
    }

    return CommandState(true, false);
}

async::Channel<Command, CommandState> AppshellCommandsState::commandStateChanged() const
{
    return m_commandStateChanged;
}
