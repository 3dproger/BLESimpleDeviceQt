#include "mainwindow.h"
#include "ui_mainwindow.h"

namespace
{

static const int UpdateValuesInterval = 200;

}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    BLESimpleDevice::TargetMeasurementData tmd;
    tmd.servicesAndCharacteristics =
    {
        {
            QBluetoothUuid((quint16)0x1101),
            {
                QBluetoothUuid((quint16)0x2101),
                QBluetoothUuid((quint16)0x2102),
                QBluetoothUuid((quint16)0x2103),
                QBluetoothUuid((quint16)0x2104),
                QBluetoothUuid((quint16)0x2105),
                QBluetoothUuid((quint16)0x2110),
                QBluetoothUuid((quint16)0x2111),
            }
        }
    };

    tmd.characteristicNames.insert(QBluetoothUuid((quint16)0x2101), "finger_1");
    tmd.characteristicNames.insert(QBluetoothUuid((quint16)0x2102), "finger_2");
    tmd.characteristicNames.insert(QBluetoothUuid((quint16)0x2103), "finger_3");
    tmd.characteristicNames.insert(QBluetoothUuid((quint16)0x2104), "finger_4");
    tmd.characteristicNames.insert(QBluetoothUuid((quint16)0x2105), "finger_5");
    tmd.characteristicNames.insert(QBluetoothUuid((quint16)0x2110), "imu_x");
    tmd.characteristicNames.insert(QBluetoothUuid((quint16)0x2111), "imu_y");

    glove = new BLESimpleDevice(QBluetoothAddress("30:7B:F5:33:2B:9D"), tmd, this);

    connect(&timerUpdateValues, &QTimer::timeout, this, &MainWindow::UpdateValues);
    timerUpdateValues.setInterval(UpdateValuesInterval);
    timerUpdateValues.start();

    ui->tableWidgetValues->setRowCount(7);
    ui->tableWidgetValues->setColumnCount(2);
    ui->tableWidgetValues->setColumnWidth(1, 800);

    for (int i = 0; i < ui->tableWidgetValues->rowCount(); ++i)
    {
        for (int j = 0; j < ui->tableWidgetValues->columnCount(); ++j)
        {
            ui->tableWidgetValues->setItem(i, j, new QTableWidgetItem());
        }
    }

    ui->tableWidgetValues->item(0, 0)->setText("finger_1");
    ui->tableWidgetValues->item(1, 0)->setText("finger_2");
    ui->tableWidgetValues->item(2, 0)->setText("finger_3");
    ui->tableWidgetValues->item(3, 0)->setText("finger_4");
    ui->tableWidgetValues->item(4, 0)->setText("finger_5");
    ui->tableWidgetValues->item(5, 0)->setText("imu_x");
    ui->tableWidgetValues->item(6, 0)->setText("imu_y");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::UpdateValues()
{
    ui->tableWidgetValues->item(0, 1)->setText(QString("%1").arg(glove->measuredValueUInt8("finger_1")));
    ui->tableWidgetValues->item(1, 1)->setText(QString("%1").arg(glove->measuredValueUInt8("finger_2")));
    ui->tableWidgetValues->item(2, 1)->setText(QString("%1").arg(glove->measuredValueUInt8("finger_3")));
    ui->tableWidgetValues->item(3, 1)->setText(QString("%1").arg(glove->measuredValueUInt8("finger_4")));
    ui->tableWidgetValues->item(4, 1)->setText(QString("%1").arg(glove->measuredValueUInt8("finger_5")));

    {
        QString text;
        const uchar value = glove->measuredValueUInt8("imu_x");
        if      (value == 1) text   = u8"Плоскость Y";
        else if (value == 2) text   = u8"Наклон руки вниз";
        else if (value == 3) text   = u8"Вниз";
        else if (value == 4) text   = u8"Наклон руки вверх";
        else if (value == 5) text   = u8"Вверх";

        text += " (" + QString("%1").arg(value) + ")";

        ui->tableWidgetValues->item(5, 1)->setText(text);
    }

    {
        QString text;
        const uchar value = glove->measuredValueUInt8("imu_y");
        if      (value == 6)  text  = u8"Плоскость X";
        else if (value == 7)  text  = u8"Наклон влево";
        else if (value == 8)  text  = u8"Лево";
        else if (value == 9)  text  = u8"Наклон вправо";
        else if (value == 10) text  = u8"Право";

        text += " (" + QString("%1").arg(value) + ")";

        ui->tableWidgetValues->item(6, 1)->setText(text);
    }

    switch (glove->GetState())
    {
    case BLESimpleDevice::Unknown:
        ui->labelInfo->setText(u8"Неизвестное состояние");
        break;
    case BLESimpleDevice::BluetoothNotAvailable:
        ui->labelInfo->setText(u8"Bluetooth не доступен на этом устройстве");
        break;
    case BLESimpleDevice::BluetoothNotEnabled:
        ui->labelInfo->setText(u8"Bluetooth не включён");
        break;
    case BLESimpleDevice::AndroidMaybeNoLocationPermitionError:
        ui->labelInfo->setText(u8"Ошибка! Проверьте, разрешён ли доступ к данным о местоположении Android-устройства и перезапустите приложение");
        break;
    case BLESimpleDevice::NotConnected:
         ui->labelInfo->setText(u8"Не подключено");
        break;
    case BLESimpleDevice::DiscoveringDevice:
        ui->labelInfo->setText(u8"Поиск устройства...");
        break;
    case BLESimpleDevice::DeviceFoundWaitToServicesDiscovering:
    case BLESimpleDevice::DiscoveringServices:
        ui->labelInfo->setText(u8"Получение сервисов...");
        break;
    case BLESimpleDevice::ServicesDiscoveredAndDiscoveringDetails:
        ui->labelInfo->setText(u8"Получение характристик...");
        break;
    case BLESimpleDevice::Connected:
        ui->labelInfo->setText(u8"Успешно подключено");
        break;
    }
}

