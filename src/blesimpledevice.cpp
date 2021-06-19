#include "blesimpledevice.h"
#include <QDebug>
#include <QBluetoothUuid>
#include <QDataStream>

namespace
{

const static int UpdateDeviceTimerInterval = 2000;
const static int LowEnergyDiscoveryTimeout = 5000;

}

BLESimpleDevice::BLESimpleDevice(const QBluetoothAddress& targetDeviceAddress_, const TargetMeasurementData& targetMeasurementData_, QObject *parent)
    : QObject(parent)
    , targetDeviceAddress(targetDeviceAddress_)
    , targetMeasurementData(targetMeasurementData_)
{
    deviceDiscoveryAgent.setLowEnergyDiscoveryTimeout(LowEnergyDiscoveryTimeout);

    connect(&deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &BLESimpleDevice::OnDeviceDiscovered);
    connect(&deviceDiscoveryAgent, static_cast<void (QBluetoothDeviceDiscoveryAgent::*)(QBluetoothDeviceDiscoveryAgent::Error)>(&QBluetoothDeviceDiscoveryAgent::error), this, &BLESimpleDevice::OnDeviceDiscoverScanError);
    connect(&deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, &BLESimpleDevice::OnDeviceDiscoverFinished);
    connect(&deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::canceled, this, &BLESimpleDevice::OnDeviceDiscoverCanceled);

    connect(&timerUpdate, &QTimer::timeout, this, &BLESimpleDevice::UpdateDevice);
    timerUpdate.setInterval(UpdateDeviceTimerInterval);
    timerUpdate.start();

    if (localDevice.hostMode() == QBluetoothLocalDevice::HostPoweredOff)
    {
        localDevice.powerOn();
    }

    DisconnectAndReset();
    UpdateDevice();
}

BLESimpleDevice::State BLESimpleDevice::GetState() const
{
#if !defined(Q_OS_WIN) and !defined(Q_OS_IOS)
    if (!localDevice.isValid())
    {
        return State::BluetoothNotAvailable;
    }

    if (localDevice.hostMode() == QBluetoothLocalDevice::HostPoweredOff)
    {
        return State::BluetoothNotEnabled;
    }
#endif

    if (!targetDeviceFound)
    {
        if (!deviceDiscoveryAgent.isActive())
        {
#ifdef Q_OS_ANDROID
            if (androidMaybeNoLocationPermitionError)
            {
                return State::AndroidMaybeNoLocationPermitionError;
            }
#endif

            return State::NotConnected;
        }
        else
        {
            return State::DiscoveringDevice;
        }
    }
    else
    {
        if (!bleController)
        {
            return State::DeviceFoundWaitToServicesDiscovering;
        }
        else
        {
            if (!serviceDiscoverFinished)
            {
                return State::DiscoveringServices;
            }
            else if (!someDescriptorWritten)
            {
                return State::ServicesDiscoveredAndDiscoveringDetails;
            }
            else
            {
                return State::Connected;
            }
        }
    }

    return State::Unknown;
}

QByteArray BLESimpleDevice::measuredValueBA(const QString &name, const QByteArray &defaultValue, bool* ok) const
{
    const auto it = measuredData.find(name);

    if (ok)
    {
        *ok = it != measuredData.end();
    }

    if (it != measuredData.end())
    {
        return *it;
    }

    return defaultValue;
}

quint8 BLESimpleDevice::measuredValueUInt8(const QString &name, const quint8& defaultValue, bool* ok) const
{
    const auto it = measuredData.find(name);

    if (ok)
    {
        *ok = it != measuredData.end();
    }

    if (it != measuredData.end())
    {
        QDataStream dataStream(*it);
        quint8 value;
        dataStream >> value;

        return value;
    }

    return defaultValue;
}

qint16 BLESimpleDevice::measuredValueInt16(const QString &name, const qint16 &defaultValue, bool *ok) const
{
    const auto it = measuredData.find(name);

    if (ok)
    {
        *ok = it != measuredData.end();
    }

    if (it != measuredData.end())
    {
        QDataStream dataStream(*it);
        qint16 value;
        dataStream >> value;

        return value;
    }

    return defaultValue;
}

qint32 BLESimpleDevice::measuredValueInt32(const QString &name, const qint32 &defaultValue, bool *ok) const
{
    const auto it = measuredData.find(name);

    if (ok)
    {
        *ok = it != measuredData.end();
    }

    if (it != measuredData.end())
    {
        QDataStream dataStream(*it);
        qint32 value;
        dataStream >> value;

        return value;
    }

    return defaultValue;
}

quint32 BLESimpleDevice::measuredValueUInt32(const QString &name, const quint32 &defaultValue, bool *ok) const
{
    const auto it = measuredData.find(name);

    if (ok)
    {
        *ok = it != measuredData.end();
    }

    if (it != measuredData.end())
    {
        QDataStream dataStream(*it);
        quint32 value;
        dataStream >> value;

        return value;
    }

    return defaultValue;
}

double BLESimpleDevice::measuredValueDouble(const QString &name, const double &defaultValue, bool *ok) const
{
    const auto it = measuredData.find(name);

    if (ok)
    {
        *ok = it != measuredData.end();
    }

    if (it != measuredData.end())
    {
        QDataStream dataStream(*it);
        double value;
        dataStream >> value;

        return value;
    }

    return defaultValue;
}

void BLESimpleDevice::OnDeviceDiscovered(const QBluetoothDeviceInfo& deviceInfo)
{
    qDebug() << "device discovered:" << deviceInfo.address() << ", name:" << deviceInfo.name() << ", rssi:" << deviceInfo.rssi();

    QString msgUuids;
    for (const QBluetoothUuid& uuid : deviceInfo.serviceUuids())
    {
        msgUuids += uuid.toString() + " ";
    }

    qDebug() << "services UUIDs (" << deviceInfo.serviceUuids().count() << "):" << msgUuids;

    if (deviceInfo.address() == targetDeviceAddress)
    {
        qDebug() << "found target device";

        targetDeviceFound = true;
        deviceDiscoveryAgent.stop();

        StartServiceDiscovery(deviceInfo);
    }

    emit DeviceChanged();
}

void BLESimpleDevice::OnDeviceDiscoverScanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    qDebug() << "device discover error:" << error;

    if (error == QBluetoothDeviceDiscoveryAgent::Error::UnknownError)
    {
#ifdef Q_OS_ANDROID
        androidMaybeNoLocationPermitionError = true;
        DisconnectAndReset();
#endif
    }

    emit DeviceChanged();
}

void BLESimpleDevice::OnDeviceDiscoverFinished()
{
    qDebug() << "OnDeviceDiscoverFinished";
}

void BLESimpleDevice::OnDeviceDiscoverCanceled()
{
    qDebug() << "OnDeviceDiscoverCanceled";
}

void BLESimpleDevice::OnServiceDiscovered(const QBluetoothUuid &newServiceUUID)
{
    qDebug() << "OnServiceDiscovered" << newServiceUUID.toString() << ", is target =" << targetMeasurementData.servicesAndCharacteristics.contains(newServiceUUID);

    if (!bleController)
    {
        qCritical() << Q_FUNC_INFO << "!bleController";
        return;
    }

    if (targetMeasurementData.servicesAndCharacteristics.contains(newServiceUUID))
    {
        QLowEnergyService* newService = bleController->createServiceObject(newServiceUUID, this);

        if (!newService)
        {
            qCritical() << Q_FUNC_INFO << "!newService";
            return;
        }

        connect(newService, &QLowEnergyService::stateChanged, this, &BLESimpleDevice::OnServiceStateChanged);
        connect(newService, &QLowEnergyService::characteristicChanged, this, &BLESimpleDevice::OnServiceCharacteristicChanged);
        connect(newService, &QLowEnergyService::descriptorWritten, this, &BLESimpleDevice::OnServiceDescriptorWritten);

        newService->discoverDetails();
    }

    emit DeviceChanged();
}

void BLESimpleDevice::OnServiceDiscoverFinished()
{
    qDebug() << "OnServiceDiscoverFinished";
    serviceDiscoverFinished = true;
}

void BLESimpleDevice::OnServiceStateChanged(QLowEnergyService::ServiceState newState)
{
    qDebug() << "OnServiceStateChanged, newState =" << newState;

    QLowEnergyService* service = dynamic_cast<QLowEnergyService*>(sender());
    if (!service)
    {
        qCritical() << Q_FUNC_INFO << "!service";
        return;
    }

    if (newState == QLowEnergyService::ServiceDiscovered)
    {
        const auto it = targetMeasurementData.servicesAndCharacteristics.find(service->serviceUuid());
        if (it == targetMeasurementData.servicesAndCharacteristics.end())
        {
            qDebug() << "ignore service" << service->serviceUuid();
            return;
        }

        qDebug() << "service details discovered";

        const QSet<QBluetoothUuid>& targetCharacteristics = *it;

        for (const QBluetoothUuid& charUUID : targetCharacteristics)
        {
            const QLowEnergyCharacteristic hrChar = service->characteristic(charUUID);
            if (!hrChar.isValid())
            {
                qDebug() << "characteristic" << charUUID.toString() << "not found";
                continue;
            }

            const QLowEnergyDescriptor notificationDesc = hrChar.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
            if (notificationDesc.isValid())
            {
                service->writeDescriptor(notificationDesc, QByteArray::fromHex("0100"));
            }
        }

        emit DeviceChanged();
    }
}

void BLESimpleDevice::OnServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &rawValue)
{
#ifdef QT_DEBUG
    qDebug() << "OnServiceCharacteristicChanged" << CharacteristicNameOrUUID(characteristic.uuid()) << ", value =" << rawValue.toHex();
#endif

    const auto it = targetMeasurementData.characteristicNames.find(characteristic.uuid());
    if (it != targetMeasurementData.characteristicNames.end())
    {
        measuredData.insert(*it, rawValue);
    }
}

void BLESimpleDevice::OnServiceDescriptorWritten(const QLowEnergyDescriptor &descriptor, const QByteArray &newValue)
{
    Q_UNUSED(descriptor)
    Q_UNUSED(newValue)
    qDebug() << "OnServiceDescriptorWritten";

    someDescriptorWritten = true;

    emit DeviceChanged();
}

void BLESimpleDevice::StartDeviceDiscovery()
{
    qDebug() << "start discovery target device: " << targetDeviceAddress;

    DisconnectAndReset();

    if (!deviceDiscoveryAgent.isActive())
    {
        deviceDiscoveryAgent.start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    }

    emit DeviceChanged();
}

void BLESimpleDevice::StartServiceDiscovery(const QBluetoothDeviceInfo &deviceInfo)
{
    if (!bleController)
    {
        bleController = QLowEnergyController::createCentral(deviceInfo, this);

        connect(bleController, &QLowEnergyController::serviceDiscovered, this, &BLESimpleDevice::OnServiceDiscovered);
        connect(bleController, &QLowEnergyController::discoveryFinished, this, &BLESimpleDevice::OnServiceDiscoverFinished);

        connect(bleController, static_cast<void (QLowEnergyController::*)(QLowEnergyController::Error)>(&QLowEnergyController::error),
                this, [this](QLowEnergyController::Error error) {
            qDebug() << "services discovery error:" << error;
            DisconnectAndReset();
            UpdateDevice();
        });

        connect(bleController, &QLowEnergyController::connected, this, [this]() {
            qDebug() << "QLowEnergyController connected. Search services...";
            bleController->discoverServices();
        });

        connect(bleController, &QLowEnergyController::disconnected, this, [this]() {
            qDebug() << "LowEnergy controller disconnected";
            DisconnectAndReset();
            UpdateDevice();
        });
    }

    bleController->connectToDevice();
}

void BLESimpleDevice::UpdateDevice()
{
    switch (GetState()) {
    case BLESimpleDevice::BluetoothNotAvailable:
        break;
    case BLESimpleDevice::BluetoothNotEnabled:
        break;
    case BLESimpleDevice::AndroidMaybeNoLocationPermitionError:
        break;
    case BLESimpleDevice::Unknown:
        break;
    case BLESimpleDevice::NotConnected:
        StartDeviceDiscovery();
        break;
    case BLESimpleDevice::DiscoveringDevice:
        break;
    case BLESimpleDevice::DeviceFoundWaitToServicesDiscovering:
        break;
    case BLESimpleDevice::DiscoveringServices:
        break;
    case BLESimpleDevice::ServicesDiscoveredAndDiscoveringDetails:
        break;
    case BLESimpleDevice::Connected:
        break;
    }
}

QString BLESimpleDevice::CharacteristicNameOrUUID(const QBluetoothUuid& uuid)
{
    const auto it = targetMeasurementData.characteristicNames.find(uuid);
    if (it != targetMeasurementData.characteristicNames.end() && !it->isEmpty())
    {
        return *it;
    }

    return uuid.toString();
}

void BLESimpleDevice::DisconnectAndReset()
{
    if (deviceDiscoveryAgent.isActive())
    {
        deviceDiscoveryAgent.stop();
    }

    if (bleController)
    {
        bleController->disconnectFromDevice();
    }

    measuredData.clear();

    targetDeviceFound = false;
    serviceDiscoverFinished = false;
    someDescriptorWritten = false;
    androidMaybeNoLocationPermitionError = false;
}
