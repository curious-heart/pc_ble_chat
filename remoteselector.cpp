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
#include "types_and_defs.h"
#include "remoteselector.h"
#include "ui_remoteselector.h"
#include "logger.h"

#include <QtBluetooth/qbluetoothlocaldevice.h>
#include <QtBluetooth/qbluetoothservicediscoveryagent.h>

QT_USE_NAMESPACE

#if (QT_VERSION < QT_VERSION_CHECK(6,0,0))
#define BLE_SRV_ST_DISC_REQD DiscoveryRequired
#define BLE_SRV_ST_DISC_ING DiscoveringServices
#define BLE_SRV_ST_DISC_ED ServiceDiscovered
#else
#define BLE_SRV_ST_DISC_REQD RemoteService
#define BLE_SRV_ST_DISC_ING RemoteServiceDiscovering
#define BLE_SRV_ST_DISC_ED RemoteServiceDiscovered
#endif

RemoteSelector::RemoteSelector(const QBluetoothAddress &/*localAdapter*/,
                           const QMap<QString, setting_ble_dev_info_t*> & cfg_dev_list,
                           QWidget *parent)
    :   QDialog(parent), ui(new Ui::RemoteSelector), m_intersted_devs(cfg_dev_list)
{
    ui->setupUi(this);
    ui->ScanServiceBtn->setEnabled(false);

    m_dev_discoveryAgent = new QBluetoothDeviceDiscoveryAgent();
    if(!m_dev_discoveryAgent)
    {
        QString err = QString("New QBluetoothDeviceDiscoveryAgent fail!");
        DIY_LOG(LOG_LEVEL::LOG_ERROR, "%ls", err.utf16());
        QMessageBox::critical(nullptr, "!!!", err);
        return;
    }

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
    connect(m_dev_discoveryAgent,
        #if (QT_VERSION < QT_VERSION_CHECK(6,2,0))
            QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::error),
        #else
            &QBluetoothDeviceDiscoveryAgent::errorOccurred,
        #endif
            this,
            &RemoteSelector::dev_discoveryErr);
}

RemoteSelector::~RemoteSelector()
{
    clear_ble_resource();
    delete m_dev_discoveryAgent;
    delete ui;
}

void RemoteSelector::startDiscovery()
{
    ui->status->setText(tr("Scanning..."));
    if (m_dev_discoveryAgent->isActive())
        m_dev_discoveryAgent->stop();

    ui->remoteDevices->clear();
    //m_target_device_found = nullptr; //false;
    m_target_dev_setting_info = nullptr;
    DIY_LOG(LOG_LEVEL::LOG_INFO, "Begin to scan device...");
    m_dev_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    //m_dev_discoveryAgent->start();
    ui->remoteDevices->setEnabled(false);
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
    setting_ble_dev_info_t * setting_info;

    dev_name = deviceInfo.name();
    dev_addr = deviceInfo.address().toString();
    setting_info = m_intersted_devs.value(dev_addr, nullptr);
    //if(m_all_dev_scan || (m_intersted_dev_addr_str == dev_addr))
    if(m_all_dev_scan || setting_info != nullptr)
    {
        QString ind;
        if(setting_info != nullptr)
        {
            ind = QString("**");
        }

        QListWidgetItem *item =
            new QListWidgetItem(QString::fromLatin1("%1 %2 %3").arg(ind,
                                                                    dev_name,
                                                                    dev_addr));
        m_discoveredDevices.insert(item, deviceInfo);
        ui->remoteDevices->addItem(item);

        DIY_LOG(LOG_LEVEL::LOG_INFO, "Find device: %ls", dev_addr.utf16());
        /*
        if(!m_all_dev_scan)
        {
            m_dev_discoveryAgent->stop();
        }*/
    }
}

void RemoteSelector::dev_discoveryFinished()
{
    QString title_str, result_str;
    int cnt = m_discoveredDevices.count();
    //if(m_target_device_found)
    if(cnt > 0)
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
    DIY_LOG(LOG_LEVEL::LOG_INFO, "%ls. cnt = %d", result_str.utf16(), cnt);
    QMessageBox::information(nullptr, title_str, result_str);
    ui->remoteDevices->setEnabled(true);
    //if(!m_target_device_found)
    if(cnt <= 0)
    {
        reject();
    }
}

void RemoteSelector::dev_discoveryErr(QBluetoothDeviceDiscoveryAgent::Error /*error*/)
{
    DIY_LOG(LOG_LEVEL::LOG_ERROR,
            "Error in QBluetoothDeviceDiscoveryAgent %ls: ",
            m_dev_discoveryAgent->errorString().utf16());
}

void RemoteSelector::on_remoteDevices_itemActivated(QListWidgetItem *item)
{
    QString dev_address;
    bool default_dev_setting = false;

    m_device = m_discoveredDevices.value(item);
    if (m_dev_discoveryAgent->isActive())
        m_dev_discoveryAgent->stop();
    dev_address = m_device.address().toString();

    m_target_dev_setting_info = m_intersted_devs.value(dev_address, nullptr);
    if(nullptr == m_target_dev_setting_info)
    {
        QString err1 = QString("选择了一个配置文件中不存在的设备");
        QString err2 = QString("尝试使用默认配置。");
        DIY_LOG(LOG_LEVEL::LOG_INFO, "%ls%ls,%ls",
                err1.utf16(), dev_address.utf16(), err2.utf16());
        m_target_dev_setting_info = m_intersted_devs.value(g_def_ble_dev_addr, nullptr);
        if(nullptr == m_target_dev_setting_info)
        {
            QString err3 = QString("获取默认设置失败！！！");
            DIY_LOG(LOG_LEVEL::LOG_ERROR, "%ls", err3.utf16());
            QMessageBox::critical(nullptr, "!!!", err1 + dev_address + "，" + err2
                                                 + "\n" + err3);
            return;
        }
        else
        {
            DIY_LOG(LOG_LEVEL::LOG_INFO, "获取默认配置成功！");
            default_dev_setting = true;
        }
        /*
        QMessageBox::information(nullptr, "!!!", "不是有效设备!");
        return;
        */
    }

    if(default_dev_setting)
    {
        m_intersted_dev_addr_str = dev_address;
    }
    else
    {
        m_intersted_dev_addr_str = m_target_dev_setting_info->addr;
    }
    m_intrested_srv_uuid_str = m_target_dev_setting_info->srv_uuid;
    m_intersted_char_tx_uuid
            = QBluetoothUuid(m_target_dev_setting_info->tx_char_uuid);
    m_intersted_char_rx_uuid
            = QBluetoothUuid(m_target_dev_setting_info->rx_char_uuid);

    clear_ble_resource();

    if(!controller)
    {
        controller = QLowEnergyController::createCentral(m_device);
        if(!controller)
        {
            QString err = QString("Create BLE Controller Error!");
            DIY_LOG(LOG_LEVEL::LOG_ERROR, "%ls", err.utf16());
            QMessageBox::critical(nullptr, "!!!", err);
            return;
        }
        connect(controller, &QLowEnergyController::connected,
                this, &RemoteSelector::deviceConnected);
        connect(controller,
        #if (QT_VERSION < QT_VERSION_CHECK(6,2,0))
                QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error),
        #else
                &QLowEnergyController::errorOccurred,
        #endif
                this, &RemoteSelector::ControllerErrorReceived);
        connect(controller, &QLowEnergyController::disconnected,
                this, &RemoteSelector::deviceDisconnected);
        connect(controller, &QLowEnergyController::serviceDiscovered,
                this, &RemoteSelector::addLowEnergyService);
        connect(controller, &QLowEnergyController::discoveryFinished,
                this, &RemoteSelector::serviceScanDone);
    }
    ui->remoteDevices->setEnabled(false);
    ui->status->setText(QString("正在连接设备..."));
    DIY_LOG(LOG_LEVEL::LOG_INFO, "Begin to connect to device...");
    controller->setRemoteAddressType(QLowEnergyController::PublicAddress);
    controller->connectToDevice();

    //accept();
}

void RemoteSelector::deviceConnected()
{
    DIY_LOG(LOG_LEVEL::LOG_INFO, "Device connected!");
    ui->status->setText(QString("正在搜索服务..."));
    DIY_LOG(LOG_LEVEL::LOG_INFO, "Begin to discover service...");
    controller->discoverServices();
}

void RemoteSelector::deviceDisconnected()
{
    DIY_LOG(LOG_LEVEL::LOG_INFO, "Disconnect from device");

    ui->ScanServiceBtn->setEnabled(false);
    emit disconnected();
}

void RemoteSelector::addLowEnergyService(const QBluetoothUuid &serviceUuid)
{
    DIY_LOG(LOG_LEVEL::LOG_INFO,
            "Discover a service: %ls", serviceUuid.toString().utf16());

    QLowEnergyService *service = controller->createServiceObject(serviceUuid);
    if (!service) {
        DIY_LOG(LOG_LEVEL::LOG_ERROR, "Cannot create service for uuid!");
        return;
    }
    //A new service is discovered, now save it.
    auto serv = new ServiceInfo(service);
    if(!serv)
    {
        DIY_LOG(LOG_LEVEL::LOG_ERROR, "Cannot new ServiceInfo!");
        return;
    }
    DIY_LOG(LOG_LEVEL::LOG_INFO, "+++++++ssssssss++++++++++++");
    DIY_LOG(LOG_LEVEL::LOG_INFO, "%ls", (ServiceInfo*)serv->getAllInfoStr().utf16());
    m_services.append(serv);
}

void RemoteSelector::serviceScanDone()
{
    bool intersted_srv_found = false;
    ServiceInfo* intersted_srv = nullptr;

    DIY_LOG(LOG_LEVEL::LOG_INFO, "-------ssssssss------------");
    DIY_LOG(LOG_LEVEL::LOG_INFO, "Service discovery finished!");
    for (auto s: qAsConst(m_services))
    {
        auto serviceInfo = qobject_cast<ServiceInfo *>(s);
        if (!serviceInfo)
            continue;

        QString srv_name,srv_type,srv_uuid, srv_state;
        srv_name = serviceInfo->getName();
        srv_type = serviceInfo->getType();
        //srv_uuid = serviceInfo->getUuid();
        srv_uuid = serviceInfo->getWholeUuid();
        srv_state = serviceInfo->getStateStr();
        DIY_LOG(LOG_LEVEL::LOG_INFO,
                "Check service %ls, state %ls",
                srv_uuid.utf16(),
                srv_state.utf16());
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
        DIY_LOG(LOG_LEVEL::LOG_INFO,
                "This service %ls is to be used.",
                m_service->getWholeUuid().utf16());
        QMessageBox::information(nullptr, "OK!",
                              QString::fromUtf8("请点击 \"建立连接\" 按钮"));
    }
    else
    {
        DIY_LOG(LOG_LEVEL::LOG_INFO, "No available service!");
        QMessageBox::information(nullptr, "!!!",
                                QString::fromUtf8("未找到合适的服务"));
        m_service = nullptr;
    }
}

void RemoteSelector::recogonize_char(QLowEnergyService * intersted_srv)
{
    const QList<QLowEnergyCharacteristic> chars = intersted_srv->characteristics();
    DIY_LOG(LOG_LEVEL::LOG_INFO, "++++++++++++");
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
        DIY_LOG(LOG_LEVEL::LOG_INFO, "%ls",
                (CharacteristicInfo*)cInfo->getAllInfoStr().utf16());
    }
    DIY_LOG(LOG_LEVEL::LOG_INFO, "------------");
    QString char_uuid_str = m_intersted_char_rx->getUuid();
    QString char_property_hex
            = QString("%1").arg(m_intersted_char_rx->getCharacteristic().properties(), 4, 16, QLatin1Char('0'));
    DIY_LOG(LOG_LEVEL::LOG_INFO,
            "rx char: %ls, char_property: %ls",
            char_uuid_str.utf16(),char_property_hex.utf16());
    QListWidgetItem *item
            = new QListWidgetItem("char_rx: " + char_uuid_str + "," + char_property_hex);
    ui->remoteDevices->addItem(item);
    char_uuid_str = m_intersted_char_tx->getUuid();
    char_property_hex
            = QString("%1").arg(m_intersted_char_tx->getCharacteristic().properties(), 4, 16, QLatin1Char('0'));
    item = new QListWidgetItem("char_tx: " + char_uuid_str + "," + char_property_hex);
    DIY_LOG(LOG_LEVEL::LOG_INFO,
            "tx char: %ls, char_property: %ls",
            char_uuid_str.utf16(),char_property_hex.utf16());
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
    DIY_LOG(LOG_LEVEL::LOG_INFO,
            "Service Details discovered, new state: %ls",
            ServiceInfo::getStateStr(newState).utf16());
    if (newState != QLowEnergyService::BLE_SRV_ST_DISC_ED) {
        // do not hang in "Scanning for characteristics" mode forever
        // in case the service discovery failed
        // We have to queue the signal up to give UI time to even enter
        // the above mode
        if (newState != QLowEnergyService::BLE_SRV_ST_DISC_ING)
        {
            QMetaObject::invokeMethod(this, "characteristicsUpdated",
                                      Qt::QueuedConnection);
        }

        ui->ScanServiceBtn->setEnabled(true);
        return;
    }
    auto service = qobject_cast<QLowEnergyService *>(sender());
    if (!service)
    {
        DIY_LOG(LOG_LEVEL::LOG_ERROR, "Service Details discovered, but get sender fail!");
        ui->ScanServiceBtn->setEnabled(true);
        return;
    }

    DIY_LOG(LOG_LEVEL::LOG_INFO,"Now recogonize characteristic...");
    recogonize_char(service);
    QMessageBox::information(nullptr, "OK!", "连接建立完成，可以开始采集数据");
    accept();
}

void RemoteSelector::ControllerErrorReceived(QLowEnergyController::Error /*error*/)
{
    DIY_LOG(LOG_LEVEL::LOG_ERROR, "Error in QLowEnergyController: %ls", controller->errorString().utf16());
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
        if(m_service->service()->state() == QLowEnergyService::BLE_SRV_ST_DISC_REQD)
        {
            DIY_LOG(LOG_LEVEL::LOG_INFO,
                    "Service Discovery Required, now begin to discovery details...");
            ui->ScanServiceBtn->setEnabled(false);
            connect(m_service->service(),
                    &QLowEnergyService::stateChanged,
                    this,
                    &RemoteSelector::serviceDetailsDiscovered);
            m_service->service()->discoverDetails();
            return;
        }
        DIY_LOG(LOG_LEVEL::LOG_INFO, "No discovery is required!");
        recogonize_char(m_service->service());
        QMessageBox::information(nullptr, "OK!", "连接建立完成，可以开始采集数据");
    }
    else
    {
        QString err = QString("No intersted service can be scanned.");
        DIY_LOG(LOG_LEVEL::LOG_WARN, "%ls", err.utf16());
        QMessageBox::information(nullptr, "!!!", err);
    }

}

void RemoteSelector::clear_ble_resource()
{
    DIY_LOG(LOG_LEVEL::LOG_INFO, __FUNCTION__);
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
