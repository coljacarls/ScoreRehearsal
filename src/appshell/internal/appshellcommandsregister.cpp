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

#include "appshellcommandsregister.h"

#include "../appshellcommands.h"
#include "rcommand/commandtypes.h"

using namespace muse;
using namespace muse::rcommand;
using namespace mu::appshell;

static const std::vector<CommandInfo> s_commandInfos = {
    CommandInfo(
        APP_QUIT_COMMAND,
        TranslatableString("action", "Quit"),
        TranslatableString("action", "Quit"),
        InputSchema(),
        Decoration(),
        Availability::All
        ),
    CommandInfo(
        APP_FULLSCREEN_COMMAND,
        TranslatableString("action", "&Full screen"),
        TranslatableString("action", "Full screen"),
        InputSchema(),
        Decoration(rcommand::Checkable::Yes),
        Availability::All
        ),
    CommandInfo(
        APP_ABOUT_COMMAND,
        TranslatableString("action", "&About ScoreRehearsal…"),
        TranslatableString("action", "About ScoreRehearsal"),
        InputSchema(),
        Decoration(),
        Availability::Dispatch
        ),
};

std::string AppshellCommandsRegister::moduleName() const
{
    return "appshell";
}

const std::vector<Command>& AppshellCommandsRegister::commandList() const
{
    static std::vector<Command> commands;
    if (commands.empty()) {
        commands.reserve(s_commandInfos.size());
        for (const auto& info : s_commandInfos) {
            commands.push_back(info.command);
        }
    }
    return commands;
}

const std::vector<CommandInfo>& AppshellCommandsRegister::commandInfoList() const
{
    return s_commandInfos;
}
