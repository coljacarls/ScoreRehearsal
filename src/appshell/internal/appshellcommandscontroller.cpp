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

#include "appshellcommandscontroller.h"

#include <QCoreApplication>
#include <QEvent>
#include <QFileOpenEvent>
#include <QWindow>

#include "actions/actiontypes.h"
#include "async/async.h"
#include "playback/playbackcommands.h"
#include "project/projectcommands.h"
#include "project/types/projecturis.h"
#include "rcommand/actiontocommand.h"

#include "../appshellcommands.h"

#include "log.h"

using namespace mu::appshell;
using namespace muse;
using namespace muse::rcommand;

void AppshellCommandsController::preInit()
{
    qApp->installEventFilter(this);
}

void AppshellCommandsController::init()
{
    auto cd = commandDispatcher();
    cd->onRequest(this, APP_QUIT_COMMAND, [this](const Params&) { return quit(); });
    cd->onRequest(this, APP_FULLSCREEN_COMMAND, [this]() { toggleFullScreen(); return muse::make_ok(); });
    cd->onRequest(this, APP_ABOUT_COMMAND, [this]() { openAboutDialog(); return muse::make_ok(); });

    // compat: action codes
    {
        static const std::vector<ActionToCommand> actionToCommands = {
            { "quit", APP_QUIT_COMMAND, {} },
            { "fullscreen", APP_FULLSCREEN_COMMAND, {} },
            { "about-scorerehearsal", APP_ABOUT_COMMAND, {} },
        };

        rcommand::registerActionToCommand(this, actionToCommands, commandDispatcher(), dispatcher());
    }

    //! NOTE Closing is taken over from the project module: its close scenario asks to save
    //! modified scores, this app never saves
    cd->preDispatch().onReceive(this, [this](const Command& command, bool* allow) {
        if (command == project::PROJECT_CLOSE_COMMAND) {
            *allow = false;
            muse::async::Async::call(this, [this]() {
                closeScore();
            });
        }
    });

    //! NOTE The MuseScore modules open their own pages (the project module opens the notation
    //! page after loading a score). The shell has only the rehearsal page, and the playback module's
    //! rehearsal mix is applied only while that page is the current one, so the shell switches back
    //! to it whenever another page was opened.
    interactive()->currentUri().ch.onReceive(this, [this](const muse::Uri& uri) {
        //! NOTE Only the pages matter; dialogs (migration, errors, ...) are opened on top of the page
        if (uri != project::NOTATION_PAGE_URI && uri != project::HOME_PAGE_URI) {
            return;
        }
        LOGI() << "page opened: " << uri.toString() << ", switching to the rehearsal page";
        muse::async::Async::call(this, [this]() {
            interactive()->open(project::NOTATION_REHEARSAL_PAGE_URI);
        });
    });
}

bool AppshellCommandsController::eventFilter(QObject* watched, QEvent* event)
{
    if ((event->type() == QEvent::Close && watched == qWindow())
        || event->type() == QEvent::Quit) {
        event->setAccepted(false);

        quit();

        return true;
    }

    if (watched == qApp && event->type() == QEvent::FileOpen) {
        const QFileOpenEvent* openEvent = static_cast<const QFileOpenEvent*>(event);
        const QUrl url = openEvent->url();

        if (openProjectScenario()->isUrlSupported(url)) {
            if (startupScenario()->startupCompleted()) {
                dispatcher()->dispatch("file-open", actions::ActionData::make_arg1<QUrl>(url));
            } else {
                startupScenario()->setStartupScoreFile(project::ProjectFile { url });
            }

            return true;
        }
    }

    return QObject::eventFilter(watched, event);
}

QWindow* AppshellCommandsController::qWindow() const
{
    return mainWindow() ? mainWindow()->qWindow() : nullptr;
}

muse::async::Promise<muse::Ret> AppshellCommandsController::closeScore()
{
    return muse::async::make_promise<Ret>([this](auto resolve) {
        if (!globalContext()->currentProject()) {
            return resolve(muse::make_ok());
        }

        if (globalContext()->playbackState()->isPlaying()) {
            commandDispatcher()->dispatch(playback::STOP_COMMAND);
        }

        interactive()->closeAllDialogs().onResolve(this, [this, resolve](const Ret&) {
            globalContext()->setCurrentProject(nullptr);
            (void)resolve(muse::make_ok());
        });

        return muse::async::Promise<Ret>::dummy_result();
    });
}

muse::async::Promise<muse::Ret> AppshellCommandsController::quit()
{
    if (m_quiting) {
        return muse::async::make_promise<Ret>([](auto resolve) {
            return resolve(muse::make_ret(Ret::Code::Busy));
        });
    }

    m_quiting = true;

    return muse::async::make_promise<Ret>([this](auto resolve) {
        closeScore().onResolve(this, [resolve](const Ret&) {
            (void)resolve(muse::make_ok());
            //! NOTE exit(), not quit(): since Qt 6.5 quit() sends a QEvent::Quit, which
            //! the event filter above turns into this very method
            QCoreApplication::exit(0);
        });

        return muse::async::Promise<Ret>::dummy_result();
    });
}

void AppshellCommandsController::toggleFullScreen()
{
    mainWindow()->toggleFullScreen();
}

void AppshellCommandsController::openAboutDialog()
{
    interactive()->open(ABOUT_URI);
}
