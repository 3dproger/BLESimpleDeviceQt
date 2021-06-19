#ifndef BLESIMPLEDEVICE_H
#define BLESIMPLEDEVICE_H

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QLowEnergyController>
#include <QBluetoothLocalDevice>
#include <QTimer>

class BLESimpleDevice : public QObject
{
    Q_OBJECT
public:
    enum State {
        Unknown = 100,
        BluetoothNotAvailable,
        BluetoothNotEnabled,
        AndroidMaybeNoLocationPermitionError,
        NotConnected,
        DiscoveringDevice,
        DeviceFoundWaitToServicesDiscovering,
        DiscoveringServices,
        ServicesDiscoveredAndDiscoveringDetails,
        Connected
    };

    struct TargetMeasurementData
    {
        QMap<QBluetoothUuid, QSet<QBluetoothUuid>> servicesAndCharacteristics; //keys = service, value = characteristic
        QHash<QBluetoothUuid, QString> characteristicNames;
    };

    explicit BLESimpleDevice(const QBluetoothAddress& targetDeviceAddress, const TargetMeasurementData& targetMeasurementData, QObject *parent = nullptr);
    State GetState() const;

    QByteArray  measuredValueBA     (const QString& name, const QByteArray& defaultValue = QByteArray(),    bool* ok = nullptr) const;
    quint8      measuredValueUInt8  (const QString& name, const quint8&     defaultValue = 0,               bool* ok = nullptr) const;
    qint16      measuredValueInt16  (const QString& name, const qint16&     defaultValue = 0,               bool* ok = nullptr) const;
    qint32      measuredValueInt32  (const QString& name, const qint32&     defaultValue = 0,               bool* ok = nullptr) const;
    quint32     measuredValueUInt32 (const QString& name, const quint32&    defaultValue = 0,               bool* ok = nullptr) const;
    double      measuredValueDouble (const QString& name, const double&     defaultValue = 0,               bool* ok = nullptr) const;

signals:
    void DeviceChanged();

private slots:
    void OnDeviceDiscovered(const QBluetoothDeviceInfo& deviceInfo);
    void OnDeviceDiscoverScanError(QBluetoothDeviceDiscoveryAgent::Error error);
    void OnDeviceDiscoverFinished();
    void OnDeviceDiscoverCanceled();

    void OnServiceDiscovered(const QBluetoothUuid &newServiceUUID);
    void OnServiceDiscoverFinished();

    void OnServiceStateChanged(QLowEnergyService::ServiceState newState);
    void OnServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue);
    void OnServiceDescriptorWritten(const QLowEnergyDescriptor &descriptor, const QByteArray &newValue);

    void StartDeviceDiscovery();
    void StartServiceDiscovery(const QBluetoothDeviceInfo& deviceInfo);
    void UpdateDevice();
    void DisconnectAndReset();

private:
    QString CharacteristicNameOrUUID(const QBluetoothUuid& uuid);

    const QBluetoothAddress targetDeviceAddress;
    const TargetMeasurementData targetMeasurementData;

    QHash<QString, QByteArray> measuredData;

    QBluetoothLocalDevice localDevice;

    QLowEnergyController *bleController = nullptr;

    QTimer timerUpdate;
    QBluetoothDeviceDiscoveryAgent deviceDiscoveryAgent;

    bool targetDeviceFound = false;
    bool serviceDiscoverFinished = false;
    bool someDescriptorWritten = false;
    bool androidMaybeNoLocationPermitionError = false;
};

#endif // BLESIMPLEDEVICE_H
