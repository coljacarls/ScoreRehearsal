/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * ScoreRehearsal
 * A lightweight, read-only rehearsal player built on MuseScore Studio components
 *
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

#include "appshellmodule.h"

#include "modularity/ioc.h"

#include "interactive/iinteractiveuriregister.h"
#include "rcommand/icommandsregister.h"
#include "rcommand/icommandsstate.h"
#include "ui/iuiconfiguration.h"
#include "ui/uitypes.h"
#include "settings.h"

#include "project/iprojectconfiguration.h"
#include "project/types/projecturis.h"

#include "appshellcommands.h"
#include "internal/appshellcommandscontroller.h"
#include "internal/appshellcommandsregister.h"
#include "internal/appshellcommandsstate.h"
#include "internal/appshellconfiguration.h"
#include "internal/startupscenario.h"

#include "log.h"

using namespace mu::appshell;
using namespace muse;
using namespace muse::modularity;

static const std::string mname("appshell");

static const muse::Settings::Key DEFAULT_THEME_APPLIED_KEY(mname, "appshell/defaultThemeApplied");

std::string AppShellModule::moduleName() const
{
    return mname;
}

void AppShellModule::registerExports()
{
    m_configuration = std::make_shared<AppShellConfiguration>(globalCtx());

    globalIoc()->registerExport<IAppShellConfiguration>(mname, m_configuration);
}

void AppShellModule::resolveImports()
{
    auto ir = globalIoc()->resolve<muse::interactive::IInteractiveUriRegister>(mname);
    if (ir) {
        //! NOTE The page URIs are the ones the MuseScore modules know: the project module
        //! opens the notation page after loading a score, the context module derives
        //! the UI context from it. The shell has a single page and shows it for all of them.
        ir->registerPageUri(project::HOME_PAGE_URI);
        ir->registerPageUri(project::NOTATION_PAGE_URI);
        ir->registerPageUri(project::NOTATION_REHEARSAL_PAGE_URI);

        ir->registerQmlUri(ABOUT_URI, "MuseScore.AppShell", "AboutDialog");
    }

    auto cr = globalIoc()->resolve<muse::rcommand::ICommandsRegister>(mname);
    if (cr) {
        cr->reg(std::make_shared<AppshellCommandsRegister>());
    }
}

void AppShellModule::onInit(const IApplication::RunMode&)
{
    m_configuration->init();

    //! NOTE Dark theme by default (on the first start; the menu can switch it later)
    settings()->setDefaultValue(DEFAULT_THEME_APPLIED_KEY, Val(false));
    if (!settings()->value(DEFAULT_THEME_APPLIED_KEY).toBool()) {
        auto uiConfiguration = globalIoc()->resolve<muse::ui::IUiConfiguration>(mname);
        if (uiConfiguration) {
            uiConfiguration->setCurrentTheme(muse::ui::DARK_THEME_CODE);
        }
        settings()->setSharedValue(DEFAULT_THEME_APPLIED_KEY, Val(true));
    }

    //! NOTE Scores saved by old MuseScore versions are shown as they were saved: no
    //! "engraving improvements" dialog, no migration. The choice is session-only (not persisted).
    auto projectConfiguration = globalIoc()->resolve<project::IProjectConfiguration>(mname);
    if (projectConfiguration) {
        //! NOTE Nothing is ever written: no autosave copies either
        projectConfiguration->setAutoSaveEnabled(false);

        for (project::MigrationType type : project::allMigrationTypes()) {
            project::MigrationOptions options;
            options.appVersion = 1; // any non-zero value makes the options valid
            options.isApplyMigration = false;
            options.isAskAgain = false;
            projectConfiguration->setMigrationOptions(type, options, false /*persistent*/);
        }
    }
}

IContextSetup* AppShellModule::newContext(const ContextPtr& ctx) const
{
    return new AppShellContext(ctx);
}

// Context

void AppShellContext::registerExports()
{
    m_commandsController = std::make_shared<AppshellCommandsController>(iocContext());

    ioc()->registerExport<IStartupScenario>(mname, new StartupScenario(iocContext()));
}

void AppShellContext::resolveImports()
{
    auto cs = ioc()->resolve<muse::rcommand::ICommandsState>(mname);
    if (cs) {
        cs->reg(std::make_shared<AppshellCommandsState>(iocContext()));
    }
}

void AppShellContext::onPreInit(const IApplication::RunMode&)
{
    m_commandsController->preInit();
}

void AppShellContext::onInit(const IApplication::RunMode& mode)
{
    if (mode == IApplication::RunMode::GuiApp) {
        m_commandsController->init();
    }
}
