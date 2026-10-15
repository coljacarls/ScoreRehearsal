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
#include <QStringList>
#include <QVariantList>
#include <qqmlintegration.h>

#include "modularity/ioc.h"
#include "async/asyncable.h"

#include "audio/iaudiodrivercontroller.h"

namespace mu::appshell {
//! Audio output settings: driver, device and buffer size. Changes apply (and persist) immediately.
class AudioSettingsModel : public QObject, public muse::Contextable, public muse::async::Asyncable
{
    Q_OBJECT

    Q_PROPERTY(QStringList audioDrivers READ audioDrivers NOTIFY audioDriverChanged)
    Q_PROPERTY(int currentAudioDriverIndex READ currentAudioDriverIndex NOTIFY audioDriverChanged)

    Q_PROPERTY(QVariantList deviceList READ deviceList NOTIFY deviceListChanged)
    Q_PROPERTY(QString currentDeviceId READ currentDeviceId NOTIFY currentDeviceIdChanged)

    Q_PROPERTY(QList<unsigned int> bufferSizeList READ bufferSizeList NOTIFY bufferSizeListChanged)
    Q_PROPERTY(unsigned int bufferSize READ bufferSize NOTIFY bufferSizeChanged)

    QML_ELEMENT

    muse::GlobalInject<muse::audio::IAudioDriverController> audioDriverController;

public:
    explicit AudioSettingsModel(QObject* parent = nullptr);

    Q_INVOKABLE void load();

    QStringList audioDrivers() const;
    int currentAudioDriverIndex() const;
    Q_INVOKABLE void selectAudioDriver(int index);

    QVariantList deviceList() const;
    QString currentDeviceId() const;
    Q_INVOKABLE void selectDevice(const QString& deviceId);

    QList<unsigned int> bufferSizeList() const;
    unsigned int bufferSize() const;
    Q_INVOKABLE void selectBufferSize(unsigned int samples);

signals:
    void audioDriverChanged();
    void deviceListChanged();
    void currentDeviceIdChanged();
    void bufferSizeListChanged();
    void bufferSizeChanged();

private:
    bool m_loaded = false;
};
}
