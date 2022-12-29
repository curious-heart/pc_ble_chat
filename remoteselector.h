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

#ifndef REMOTESELECTOR_H
#define REMOTESELECTOR_H

#include <QtWidgets/qdialog.h>
#include <QList>
//#include <QtBluetooth/qbluetoothaddress.h>
//#include <QtBluetooth/qbluetoothserviceinfo.h>
//#include <QtBluetooth/qbluetoothuuid.h>
#include <QBluetoothDeviceDiscoveryAgent> //LLLL+++
#include <QBluetoothServiceDiscoveryAgent>
#include <QBluetoothServiceInfo>
#include <QBluetoothAddress>
#include <QBluetoothUuid>
#include <QMessageBox>
#include <QLowEnergyController>
#include "serviceinfo.h"
#include "characteristicinfo.h"
#include "sw_setting_parse.h"
QT_FORWARD_DECLARE_CLASS(QBluetoothServiceDiscoveryAgent)
QT_FORWARD_DECLARE_CLASS(QListWidgetItem)

QT_FORWARD_DECLARE_CLASS(QBluetoothDeviceDiscoveryAgent)

QT_USE_NAMESPACE

QT_BEGIN_NAMESPACE
namespace Ui {
    class RemoteSelector;
}
QT_END_NAMESPACE

class RemoteSelector : public QDialog
{
    Q_OBJECT

public:
    explicit RemoteSelector(const QBluetoothAddress &localAdapter,
                    const QMap<QString, setting_ble_dev_info_t*> & cfg_dev_list,
                    QWidget *parent = nullptr);
    ~RemoteSelector();

    void startDiscovery();
    void stopDiscovery();
    ServiceInfo * intersted_service() const;
    QBluetoothDeviceInfo * current_device();
    CharacteristicInfo* rx_char();
    CharacteristicInfo* tx_char();
    void scan_char();
    /*
    const QString m_intersted_dev_addr_str = "27:A5:D2:1E:67:DD";
    const QString m_intrested_srv_uuid_str = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
    const QBluetoothUuid m_intersted_char_tx_uuid
                = QBluetoothUuid(QString("6e400002-b5a3-f393-e0a9-e50e24dcca9e"));
    const QBluetoothUuid m_intersted_char_rx_uuid
                = QBluetoothUuid(QString("6e400003-b5a3-f393-e0a9-e50e24dcca9e"));
   */
    QString m_intersted_dev_addr_str;// = "1E:A2:CA:1E:67:DD";
    QString m_intrested_srv_uuid_str; // = "0000fff0-0000-1000-8000-00805f9b34fb";
    QBluetoothUuid m_intersted_char_tx_uuid;
                //= QBluetoothUuid(QString("0000fff2-0000-1000-8000-00805f9b34fb"));
    QBluetoothUuid m_intersted_char_rx_uuid;
                //= QBluetoothUuid(QString("0000fff1-0000-1000-8000-00805f9b34fb"));
    void search_all_dev(bool all_dev);
    /*point to one of items in m_intersted_devs, corresponding to the one user
      selected from device-scan result*/
    setting_ble_dev_info_t * m_target_dev_setting_info = nullptr;

private:
    Ui::RemoteSelector *ui;

    QBluetoothDeviceDiscoveryAgent *m_dev_discoveryAgent; //LLLL++++
    QMap<QListWidgetItem *, QBluetoothDeviceInfo> m_discoveredDevices;//LLLL++++
    QBluetoothDeviceInfo m_device;
    //bool m_target_device_found = false;
    //setting_ble_dev_info_t * m_target_device_found = nullptr;
    QLowEnergyController *controller = nullptr;
    QList<QObject *> m_services;
    ServiceInfo * m_service = nullptr;

    QList<QObject *> m_characteristics;
    CharacteristicInfo *m_intersted_char_rx = nullptr;
    CharacteristicInfo *m_intersted_char_tx = nullptr;

    bool m_all_dev_scan = false;

    /*intersted device list from settings xml file.*/
    const QMap<QString, setting_ble_dev_info_t*> & m_intersted_devs;
    /* If user select a device that is not recorded in m_intersted_devs,
     * use m_work_dev_not_in_intersted_list to record that dev info. In this case,
     * m_target_dev_setting_info points to this variable.
    */
    setting_ble_dev_info_t m_work_dev_not_in_intersted_list;
private:
    void recogonize_char(QLowEnergyService * intersted_srv);
    QString format_dev_info_str(const QBluetoothDeviceInfo * dev);
    void clear_ble_resource();

private slots:
    //void serviceDiscovered(const QBluetoothServiceInfo &serviceInfo);
    void dev_discoveryFinished();
    void dev_discoveryErr(QBluetoothDeviceDiscoveryAgent::Error error);
    void on_remoteDevices_itemActivated(QListWidgetItem *item);
    //void on_cancelButton_clicked();

    void deviceFound(const QBluetoothDeviceInfo &deviceInfo);//LLLL++++
    void deviceConnected();
    void deviceDisconnected();
    void addLowEnergyService(const QBluetoothUuid &serviceUuid);
    void serviceScanDone();
    void serviceDetailsDiscovered(QLowEnergyService::ServiceState newState);
    void ControllerErrorReceived(QLowEnergyController::Error /*error*/);

    //void on_pushButton_clicked();

    void on_RScancelButton_clicked();

    void on_ScanServiceBtn_clicked();

Q_SIGNALS:
    void disconnected();
};

#endif // REMOTESELECTOR_H
