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

#include "async/asyncable.h"
#include "actions/actionable.h"
#include "rcommand/commandable.h"
#include "rcommand/commandtypes.h"

#include "modularity/ioc.h"
#include "actions/iactionsdispatcher.h"
#include "rcommand/icommanddispatcher.h"
#include "ui/imainwindow.h"
#include "interactive/iinteractive.h"
#include "context/iglobalcontext.h"
#include "project/iopenprojectscenario.h"
#include "istartupscenario.h"

class QWindow;

namespace mu::appshell {
//! Application level commands: quit, full screen, about.
//! Opening scores is handled by the project module (command://project/open); closing is
//! taken over from it (command://project/close) so that the score is never saved.
class AppshellCommandsController : public QObject, public muse::Contextable, public muse::actions::Actionable,
    public muse::async::Asyncable, public muse::rcommand::Commandable
{
    muse::ContextInject<muse::actions::IActionsDispatcher> dispatcher = { this };
    muse::ContextInject<muse::rcommand::ICommandDispatcher> commandDispatcher = { this };
    muse::ContextInject<muse::ui::IMainWindow> mainWindow = { this };
    muse::ContextInject<muse::IInteractive> interactive = { this };
    muse::ContextInject<project::IOpenProjectScenario> openProjectScenario = { this };
    muse::ContextInject<context::IGlobalContext> globalContext = { this };
    muse::ContextInject<IStartupScenario> startupScenario = { this };

public:
    AppshellCommandsController(const muse::modularity::ContextPtr& iocCtx)
        : muse::Contextable(iocCtx) {}

    void preInit();
    void init();

private:
    bool eventFilter(QObject* watched, QEvent* event) override;

    QWindow* qWindow() const;

    //! Closes the score without ever asking to save it (the app is read-only: scores saved by old
    //! MuseScore versions count as modified after loading, which must not trigger a save prompt)
    muse::async::Promise<muse::Ret> closeScore();
    muse::async::Promise<muse::Ret> quit();
    void toggleFullScreen();
    void openAboutDialog();

    bool m_quiting = false;
};
}
