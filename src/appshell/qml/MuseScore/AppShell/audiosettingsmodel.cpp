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

#include "audiosettingsmodel.h"

#include "audio/common/audiotypes.h"

using namespace mu::appshell;
using namespace muse::audio;

AudioSettingsModel::AudioSettingsModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

void AudioSettingsModel::load()
{
    if (m_loaded) {
        return;
    }
    m_loaded = true;

    audioDriverController()->currentAudioDriverChanged().onNotify(this, [this]() {
        emit audioDriverChanged();
        emit deviceListChanged();
        emit currentDeviceIdChanged();
        emit bufferSizeListChanged();
        emit bufferSizeChanged();
    });

    audioDriverController()->availableOutputDevicesChanged().onNotify(this, [this]() {
        emit deviceListChanged();
        emit currentDeviceIdChanged();
    });

    audioDriverController()->outputDeviceChanged().onNotify(this, [this]() {
        emit currentDeviceIdChanged();
        emit bufferSizeListChanged();
        emit bufferSizeChanged();
    });

    audioDriverController()->outputDeviceBufferSizeChanged().onNotify(this, [this]() {
        emit bufferSizeChanged();
    });
}

QStringList AudioSettingsModel::audioDrivers() const
{
    QStringList result;
    for (const std::string& driver : audioDriverController()->availableAudioDrivers()) {
        result << QString::fromStdString(driver);
    }
    return result;
}

int AudioSettingsModel::currentAudioDriverIndex() const
{
    return audioDrivers().indexOf(QString::fromStdString(audioDriverController()->currentAudioDriverName()));
}

void AudioSettingsModel::selectAudioDriver(int index)
{
    const std::vector<std::string> drivers = audioDriverController()->availableAudioDrivers();
    if (index < 0 || index >= static_cast<int>(drivers.size()) || index == currentAudioDriverIndex()) {
        return;
    }

    audioDriverController()->changeCurrentAudioDriver(drivers.at(index));
    emit audioDriverChanged();
}

QVariantList AudioSettingsModel::deviceList() const
{
    QVariantList result;
    for (const AudioDevice& device : audioDriverController()->availableOutputDevices()) {
        QVariantMap item;
        item["value"] = QString::fromStdString(device.id);
        item["text"] = QString::fromStdString(device.name);
        result << item;
    }
    return result;
}

QString AudioSettingsModel::currentDeviceId() const
{
    return QString::fromStdString(audioDriverController()->outputDevice());
}

void AudioSettingsModel::selectDevice(const QString& deviceId)
{
    audioDriverController()->selectOutputDevice(deviceId.toStdString());
}

QList<unsigned int> AudioSettingsModel::bufferSizeList() const
{
    QList<unsigned int> result;
    for (samples_t samples : audioDriverController()->availableOutputDeviceBufferSizes()) {
        result << static_cast<unsigned int>(samples);
    }
    return result;
}

unsigned int AudioSettingsModel::bufferSize() const
{
    return audioDriverController()->activeSpec().output.samplesPerChannel;
}

void AudioSettingsModel::selectBufferSize(unsigned int samples)
{
    audioDriverController()->changeBufferSize(samples);
}
