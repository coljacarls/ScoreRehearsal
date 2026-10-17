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
#include "webbridge.h"

#include <QList>

#ifdef Q_OS_WASM
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>

#include <emscripten/bind.h>
#include <emscripten/val.h>
#endif

#include "log.h"

using namespace mu::appshell;

#ifdef Q_OS_WASM
namespace {
//! The persistent folder of the page (src/web/storage.js, GlobalConfiguration::userDataPath)
const QString LIBRARY_DIR = QStringLiteral("/files/data/library");

//! Calls Module[name](args...) of the page; nothing happens if the page does not have the function
template<typename ... Args>
emscripten::val callPage(const char* name, Args&&... args)
{
    emscripten::val function = emscripten::val::module_property(name);
    if (function.isUndefined() || function.isNull()) {
        LOGW() << "the page has no function " << name;
        return emscripten::val::undefined();
    }

    return function(std::forward<Args>(args)...);
}

bool isPresentationMode()
{
    static const bool s_presentation = [] {
        const emscripten::val mode = emscripten::val::module_property("webMode");
        return mode.isString() && mode.as<std::string>() == "presentation";
    }();

    return s_presentation;
}

QList<WebBridge*>& bridges()
{
    static QList<WebBridge*> s_bridges;
    return s_bridges;
}

//! The page calls these two functions (Module.onPresentationState, Module.onPresentationClosed)
void onPresentationState(const std::string& json)
{
    WebBridge::deliverState(QString::fromStdString(json));
}

void onPresentationClosed()
{
    WebBridge::deliverPresentationClosed();
}
}

EMSCRIPTEN_BINDINGS(ScoreRehearsalWeb) {
    emscripten::function("onPresentationState", &onPresentationState);
    emscripten::function("onPresentationClosed", &onPresentationClosed);
}
#else
namespace {
QList<WebBridge*>& bridges()
{
    static QList<WebBridge*> s_bridges;
    return s_bridges;
}
}
#endif

bool web::isWeb()
{
#ifdef Q_OS_WASM
    return true;
#else
    return false;
#endif
}

bool web::isPresentationInstance()
{
#ifdef Q_OS_WASM
    return isPresentationMode();
#else
    return false;
#endif
}

void web::persistStorage()
{
#ifdef Q_OS_WASM
    //! The presentation tab has an old copy of the files: it must not write them over the ones of the rehearsal tab
    if (!isPresentationMode()) {
        callPage("persistStorage");
    }
#endif
}

bool web::openPresentationTab(const QString& scorePath)
{
#ifdef Q_OS_WASM
    const emscripten::val opened = callPage("openPresentationTab", scorePath.toStdString());
    return opened.isTrue();
#else
    Q_UNUSED(scorePath)
    return false;
#endif
}

void web::closePresentationTab()
{
#ifdef Q_OS_WASM
    callPage("closePresentationTab");
#endif
}

QString web::libraryPath(const QString& filePath)
{
#ifdef Q_OS_WASM
    if (!filePath.startsWith(QLatin1String("/interactive/")) && !filePath.startsWith(QLatin1String("/mu/"))) {
        return filePath;
    }

    struct Entry {
        qint64 size = 0;
        QDateTime modified;
        QString library;
    };
    static QHash<QString, Entry> s_known;

    const QFileInfo info(filePath);
    if (!info.exists()) {
        return filePath;
    }

    //! The score is looked up several times while it is open: do not read it again for every call
    const auto known = s_known.constFind(filePath);
    if (known != s_known.constEnd() && known->size == info.size() && known->modified == info.lastModified()
        && QFileInfo::exists(known->library)) {
        return known->library;
    }

    QFile source(filePath);
    if (!source.open(QIODevice::ReadOnly)) {
        return filePath;
    }

    const QByteArray data = source.readAll();
    const QString hash = QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex().left(16));
    const QString dir = LIBRARY_DIR + QLatin1Char('/') + hash;
    const QString target = dir + QLatin1Char('/') + info.fileName();

    if (!QFileInfo::exists(target)) {
        QDir().mkpath(dir);

        QFile out(target);
        if (!out.open(QIODevice::WriteOnly) || out.write(data) != data.size()) {
            LOGE() << "unable to copy the score to the library: " << target;
            return filePath;
        }

        out.close();
        persistStorage();
    }

    s_known.insert(filePath, { info.size(), info.lastModified(), target });
    return target;
#else
    return filePath;
#endif
}

WebBridge::WebBridge(QObject* parent)
    : QObject(parent)
{
    bridges().append(this);
}

WebBridge::~WebBridge()
{
    bridges().removeAll(this);
}

bool WebBridge::isWeb() const
{
    return web::isWeb();
}

bool WebBridge::isPresentationInstance() const
{
    return web::isPresentationInstance();
}

bool WebBridge::openPresentationTab(const QString& scorePath)
{
    return web::openPresentationTab(scorePath);
}

void WebBridge::closePresentationTab()
{
    web::closePresentationTab();
}

void WebBridge::publishState(const QString& json)
{
#ifdef Q_OS_WASM
    callPage("publishPresentationState", json.toStdString());
#else
    Q_UNUSED(json)
#endif
}

void WebBridge::toggleFullscreen()
{
#ifdef Q_OS_WASM
    callPage("toggleFullscreen");
#endif
}

void WebBridge::exportSessions()
{
#ifdef Q_OS_WASM
    callPage("exportSessions");
#endif
}

void WebBridge::importSessions()
{
#ifdef Q_OS_WASM
    callPage("importSessions");
#endif
}

void WebBridge::deliverState(const QString& json)
{
    //! A copy: a handler may create or delete bridges
    const QList<WebBridge*> all = bridges();
    for (WebBridge* bridge : all) {
        emit bridge->stateReceived(json);
    }
}

void WebBridge::deliverPresentationClosed()
{
    const QList<WebBridge*> all = bridges();
    for (WebBridge* bridge : all) {
        emit bridge->presentationClosed();
    }
}
