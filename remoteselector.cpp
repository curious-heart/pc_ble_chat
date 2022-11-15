/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtBluetooth module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "remoteselector.h"
#include "ui_remoteselector.h"

#include <QtBluetooth/qbluetoothlocaldevice.h>
#include <QtBluetooth/qbluetoothservicediscoveryagent.h>

QT_USE_NAMESPACE

RemoteSelector::RemoteSelector(const QBluetoothAddress &localAdapter, QWidget *parent)
    :   QDialog(parent), ui(new Ui::RemoteSelector)
{
    ui->setupUi(this);
    ui->ScanServiceBtn->setEnabled(false);

    m_dev_discoveryAgent = new QBluetoothDeviceDiscoveryAgent();
    m_dev_discoveryAgent->setLowEnergyDiscoveryTimeout(5000);
    connect(m_dev_discoveryAgent,
            &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this,
            &RemoteSelector::deviceFound);
    connect(m_dev_discoveryAgent,
            &QBluetoothDeviceDiscoveryAgent::finished,
            this,
            &RemoteSelector::dev_discoveryFinished);
    connect(m_dev_discoveryAgent,
            &QBluetoothDeviceDiscoveryAgent::canceled,
            this,
            &RemoteSelector::dev_discoveryFinished);
}

RemoteSelector::~RemoteSelector()
{
    clear_ble_resource();
    delete m_dev_discoveryAgent;
    delete ui;
}

void RemoteSelector::startDiscovery(const QBluetoothUuid &uuid)
{
    ui->status->setText(tr("Scanning..."));
    if (m_dev_discoveryAgent->isActive())
        m_dev_discoveryAgent->stop();

    ui->remoteDevices->clear();
    m_target_device_found = false;
    m_dev_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    //m_dev_discoveryAgent->start();
}

void RemoteSelector::stopDiscovery()
{
    if (m_dev_discoveryAgent){
        m_dev_discoveryAgent->stop();
    }
}
QBluetoothDeviceInfo * RemoteSelector::current_device()
{
    return &m_device;
}

ServiceInfo* RemoteSelector::intersted_service() const
{
    return m_service;
}
CharacteristicInfo * RemoteSelector::rx_char()
{
    return m_intersted_char_rx;
}
CharacteristicInfo * RemoteSelector::tx_char()
{
    return m_intersted_char_tx;
}

void RemoteSelector::deviceFound(const QBluetoothDeviceInfo &deviceInfo)
{
    QString dev_name, dev_addr;

    dev_name = deviceInfo.name();
    dev_addr = deviceInfo.address().toString();

    if(m_all_dev_scan || (m_intersted_dev_addr_str == dev_addr))
    {
        QListWidgetItem *item =
            new QListWidgetItem(QString::fromLatin1("%1 %2").arg(dev_name,
                                                             dev_addr));
        m_discoveredDevices.insert(item, deviceInfo);
        ui->remoteDevices->addItem(item);
        m_target_device_found = true;

        if(!m_all_dev_scan)
        {
            m_dev_discoveryAgent->stop();
        }
    }
}

void RemoteSelector::dev_discoveryFinished()
{
    QString title_str, result_str;

    if(m_target_device_found)
    {
        ui->status->setText(QString::fromUtf8("请双击需要连接的设备:"));
        title_str = "OK!";
        result_str = "设备搜索完成";
    }
    else
    {
        title_str = "!!!";
        result_str = "没有发现设备";
    }
    QMessageBox::information(nullptr, title_str, result_str);
    if(!m_target_device_found)
    {
        reject();
    }
}

void RemoteSelector::on_remoteDevices_itemActivated(QListWidgetItem *item)
{
    qDebug() << "got click" << item->text();

    QString dev_name, dev_address;
    m_device = m_discoveredDevices.value(item);
    if (m_dev_discoveryAgent->isActive())
        m_dev_discoveryAgent->stop();
    dev_name = m_device.name();
    dev_address = m_device.address().toString();

    clear_ble_resource();

    if(!controller)
    {
        controller = QLowEnergyController::createCentral(m_device);
        if(!controller)
        {
            QMessageBox::critical(nullptr, "!!!", "Create BLE Controller Error");
            return;
        }
        connect(controller, &QLowEnergyController::connected,
                this, &RemoteSelector::deviceConnected);
        connect(controller, QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error),
                this, &RemoteSelector::errorReceived);
        connect(controller, &QLowEnergyController::disconnected,
                this, &RemoteSelector::deviceDisconnected);
        connect(controller, &QLowEnergyController::serviceDiscovered,
                this, &RemoteSelector::addLowEnergyService);
        connect(controller, &QLowEnergyController::discoveryFinished,
                this, &RemoteSelector::serviceScanDone);
    }
    ui->status->setText(QString("正在搜索服务..."));
    controller->setRemoteAddressType(QLowEnergyController::PublicAddress);
    controller->connectToDevice();

    //accept();
}

void RemoteSelector::deviceConnected()
{
    controller->discoverServices();
}

void RemoteSelector::deviceDisconnected()
{
    qWarning() << "Disconnect from device";
    ui->ScanServiceBtn->setEnabled(false);
    emit disconnected();
}

void RemoteSelector::addLowEnergyService(const QBluetoothUuid &serviceUuid)
{
    QLowEnergyService *service = controller->createServiceObject(serviceUuid);
    if (!service) {
        qWarning() << "Cannot create service for uuid";
        return;
    }
    //A new service is discovered, now save it.
    auto serv = new ServiceInfo(service);
    m_services.append(serv);
    qDebug() << "Find service:" << service->serviceUuid().toString();
}

void RemoteSelector::serviceScanDone()
{
    bool intersted_srv_found = false;
    ServiceInfo* intersted_srv = nullptr;

    for (auto s: qAsConst(m_services)) {
        auto serviceInfo = qobject_cast<ServiceInfo *>(s);
        if (!serviceInfo)
            continue;

        QString srv_name,srv_type,srv_uuid, srv_state;
        srv_name = serviceInfo->getName();
        srv_type = serviceInfo->getType();
        //srv_uuid = serviceInfo->getUuid();
        srv_uuid = serviceInfo->getWholeUuid();
        srv_state = QString("%1").arg((int)(serviceInfo->service()->state()));
        qDebug() << "Check srv:" << srv_uuid;
        if(m_intrested_srv_uuid_str == srv_uuid)
        {
            //Intersted service is found, now display only it and clear other devices.
            ui->remoteDevices->clear();
            QListWidgetItem *item;
            item = new QListWidgetItem(format_dev_info_str(&m_device));
            ui->remoteDevices->addItem(item);
            item
                = new QListWidgetItem(QString::fromLatin1("%1,%2,%3,%4").arg(srv_name, srv_type, srv_uuid, srv_state));
            ui->remoteDevices->addItem(item);
            intersted_srv = serviceInfo;
            intersted_srv_found = true;
            break;
        }
    }
    ui->status->setText(QString("服务搜索完成"));
    if(intersted_srv_found)
    {
        m_service = intersted_srv;
        ui->ScanServiceBtn->setEnabled(true);
        QMessageBox::information(nullptr, "OK!",
                              QString::fromUtf8("请点击 \"建立连接\" 按钮"));
    }
    else
    {
        QMessageBox::information(nullptr, "!!!",
                                QString::fromUtf8("未找到合适的服务"));
        m_service = nullptr;
    }
}

void RemoteSelector::recogonize_char(QLowEnergyService * intersted_srv)
{
    const QList<QLowEnergyCharacteristic> chars = intersted_srv->characteristics();
    for (const QLowEnergyCharacteristic &ch : chars)
    {
        auto cInfo = new CharacteristicInfo(ch);
        m_characteristics.append(cInfo);
        if (ch.uuid() == m_intersted_char_rx_uuid)
        {
            m_intersted_char_rx = cInfo;
        }
        if (ch.uuid() == m_intersted_char_tx_uuid)
        {
            m_intersted_char_tx = cInfo;
        }
    }
    QString char_uuid_str = m_intersted_char_rx->getUuid();
    QString char_property_hex
            = QString("%1").arg(m_intersted_char_rx->getCharacteristic().properties(), 4, 16, QLatin1Char('0'));
    qDebug() << "rx char: " + char_uuid_str + char_property_hex;
    QListWidgetItem *item
            = new QListWidgetItem("char_rx: " + char_uuid_str + "," + char_property_hex);
    ui->remoteDevices->addItem(item);
    char_uuid_str = m_intersted_char_tx->getUuid();
    char_property_hex
            = QString("%1").arg(m_intersted_char_tx->getCharacteristic().properties(), 4, 16, QLatin1Char('0'));
    item = new QListWidgetItem("char_tx: " + char_uuid_str + "," + char_property_hex);
    qDebug() << "tx char: " + char_uuid_str + char_property_hex;
    ui->remoteDevices->addItem(item);
}

QString RemoteSelector::format_dev_info_str(const QBluetoothDeviceInfo * dev)
{
    QString dev_name, dev_addr;
    dev_name = dev->name();
    dev_addr = dev->address().toString();
    return (dev_name + ", " + dev_addr);
}

void RemoteSelector::serviceDetailsDiscovered(QLowEnergyService::ServiceState newState)
{
    if (newState != QLowEnergyService::ServiceDiscovered) {
        // do not hang in "Scanning for characteristics" mode forever
        // in case the service discovery failed
        // We have to queue the signal up to give UI time to even enter
        // the above mode
        if (newState != QLowEnergyService::DiscoveringServices) {
            QMetaObject::invokeMethod(this, "characteristicsUpdated",
                                      Qt::QueuedConnection);
        }

        qDebug() << "newState is:" << newState;
        return;
    }
    //QMessageBox::information(NULL, "OK", "Service Details discovered");
    auto service = qobject_cast<QLowEnergyService *>(sender());
    if (!service)
        return;

    qDebug() << "Now recogonize characteristic...";
    recogonize_char(service);
    QMessageBox::information(nullptr, "OK!", "连接建立完成，可以开始采集数据");
    accept();
}

void RemoteSelector::errorReceived(QLowEnergyController::Error /*error*/)
{
    qWarning() << "Error: " << controller->errorString();
    //setUpdate(QString("Back\n(%1)").arg(controller->errorString()));
}

void RemoteSelector::on_RScancelButton_clicked()
{
    if (m_dev_discoveryAgent->isActive())
    {
        m_dev_discoveryAgent->stop();
    }
    clear_ble_resource();
    m_service = nullptr;
    reject();
}

void RemoteSelector::on_ScanServiceBtn_clicked()
{
    if(m_service)
    {
        if(m_service->service()->state() == QLowEnergyService::DiscoveryRequired)
        {
            connect(m_service->service(),
                    &QLowEnergyService::stateChanged,
                    this,
                    &RemoteSelector::serviceDetailsDiscovered);
            m_service->service()->discoverDetails();
            return;
        }
        QMessageBox::information(nullptr, "OK!", "连接建立完成，可以开始采集数据");
        recogonize_char(m_service->service());
    }
    else
    {
        QMessageBox::information(nullptr, "!!!", "No intersted service can be scanned.");
    }

}

void RemoteSelector::clear_ble_resource()
{
    if(controller)
    {
        controller->disconnectFromDevice();
        delete controller;
        controller = nullptr;
    }
    qDeleteAll(m_characteristics);
    m_characteristics.clear();
    m_intersted_char_rx = nullptr;
    m_intersted_char_tx = nullptr;

    qDeleteAll(m_services);
    m_services.clear();
    m_service = nullptr;
}

void RemoteSelector::search_all_dev(bool all_dev)
{
    m_all_dev_scan = all_dev;
}
/*--------------------------------------------------------------------------------*/
/*Below functions is not used currentlly.*/
void RemoteSelector::scan_char()
{
    if(m_service)
    {
        if(m_service->service()->state() == QLowEnergyService::DiscoveryRequired)
        {
            connect(m_service->service(),
                    &QLowEnergyService::stateChanged,
                    this,
                    &RemoteSelector::serviceDetailsDiscovered);
            m_service->service()->discoverDetails();
            return;
        }
        recogonize_char(m_service->service());
    }
    else
    {
        qDebug() << "No intersted service can be scanned.";
    }

}
