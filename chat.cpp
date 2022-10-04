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

#include "chat.h"
#include "chatserver.h"
#include "chatclient.h"

#include <QtCore/qdebug.h>

#include <QtBluetooth/qbluetoothdeviceinfo.h>
#include <QtBluetooth/qbluetoothlocaldevice.h>
#include <QtBluetooth/qbluetoothuuid.h>
#include <QMessageBox>
#include <QThread>

#ifdef Q_OS_ANDROID
#include <QtAndroidExtras/QtAndroid>
#endif

static const QLatin1String serviceUuid("e8e10f95-1a70-4b27-9ccf-02010264e9c8");
#ifdef Q_OS_ANDROID
static const QLatin1String reverseUuid("c8e96402-0102-cf9c-274b-701a950fe1e8");
#endif

Chat::Chat(QWidget *parent)
    : QDialog(parent), ui(new Ui_Chat)
{
    //! [Construct UI]
    ui->setupUi(this);

    connect(ui->quitButton, &QPushButton::clicked, this, &Chat::accept);
    connect(ui->connectButton, &QPushButton::clicked, this, &Chat::connectClicked);
    connect(ui->sendButton, &QPushButton::clicked, this, &Chat::sendClicked);
    connect(ui->chat, &QTextEdit::textChanged, this, &Chat::on_chat_textChanged);
    connect(ui->disconnButton, &QPushButton::clicked,
            this, &Chat::on_disconnButton_clicked);

    //! [Construct UI]

    localAdapters = QBluetoothLocalDevice::allDevices();
    if (localAdapters.count() < 2) {
        ui->localAdapterBox->setVisible(false);
    } else {
        //we ignore more than two adapters
        ui->localAdapterBox->setVisible(true);
        ui->firstAdapter->setText(tr("Default (%1)", "%1 = Bluetooth address").
                                  arg(localAdapters.at(0).address().toString()));
        ui->secondAdapter->setText(localAdapters.at(1).address().toString());
        ui->firstAdapter->setChecked(true);
        connect(ui->firstAdapter, &QRadioButton::clicked, this, &Chat::newAdapterSelected);
        connect(ui->secondAdapter, &QRadioButton::clicked, this, &Chat::newAdapterSelected);
        QBluetoothLocalDevice adapter(localAdapters.at(0).address());
        adapter.setHostMode(QBluetoothLocalDevice::HostDiscoverable);
    }

    /*
    //! [Create Chat Server]
    server = new ChatServer(this);
    connect(server, QOverload<const QString &>::of(&ChatServer::clientConnected),
            this, &Chat::clientConnected);
    connect(server, QOverload<const QString &>::of(&ChatServer::clientDisconnected),
            this,  QOverload<const QString &>::of(&Chat::clientDisconnected));
    connect(server, &ChatServer::messageReceived,
            this,  &Chat::showMessage);
    connect(this, &Chat::sendMessage, server, &ChatServer::sendMessage);
    server->startServer();
    //! [Create Chat Server]
    */
    //! [Get local device name]
    localName = QBluetoothLocalDevice().name();
    //! [Get local device name]

    init_write_data();
    connect(this, &Chat::write_data_done_sig,
            this, &Chat::write_data_done_handle);

    m_data_file_pth_str = "../" + QString(m_data_dir_rel_name);
    m_redundant_file_path_str = m_data_file_pth_str + "/"
                        + QString(m_redundant_dir_rel_name);

    m_adapter = localAdapters.isEmpty() ? QBluetoothAddress() :
                           localAdapters.at(currentAdapterIndex).address();

    ui->sendButton->setEnabled(false);
    ui->disconnButton->setEnabled(false);
}

Chat::~Chat()
{
    qDeleteAll(clients);
    delete server;
    delete m_remoteSelector;

    if(m_file_write_ready)
    {
        m_file.close();
        m_valid_data_file.close();
        m_file_write_ready = false;
    }
}

//! [clientConnected clientDisconnected]
void Chat::clientConnected(const QString &name)
{
    ui->chat->insertPlainText(QString::fromLatin1("%1 has joined chat.\n").arg(name));
}

void Chat::clientDisconnected(const QString &name)
{
    ui->chat->insertPlainText(QString::fromLatin1("%1 has left.\n").arg(name));
}
//! [clientConnected clientDisconnected]

//! [connected]
void Chat::connected(const QString &name)
{
    ui->chat->insertPlainText(QString::fromLatin1("Joined chat with %1.\n").arg(name));
}
//! [connected]

void Chat::newAdapterSelected()
{
    const int newAdapterIndex = adapterFromUserSelection();
    if (currentAdapterIndex != newAdapterIndex) {
        //server->stopServer();
        currentAdapterIndex = newAdapterIndex;
        const QBluetoothHostInfo info = localAdapters.at(currentAdapterIndex);
        QBluetoothLocalDevice adapter(info.address());
        adapter.setHostMode(QBluetoothLocalDevice::HostDiscoverable);
        //server->startServer(info.address());
        localName = info.name();
    }
}

int Chat::adapterFromUserSelection() const
{
    int result = 0;
    QBluetoothAddress newAdapter = localAdapters.at(0).address();

    if (ui->secondAdapter->isChecked()) {
        newAdapter = localAdapters.at(1).address();
        result = 1;
    }
    return result;
}

void Chat::reactOnSocketError(const QString &error)
{
    ui->chat->insertPlainText(error);
}

//! [clientDisconnected]
void Chat::clientDisconnected()
{
    ChatClient *client = qobject_cast<ChatClient *>(sender());
    if (client) {
        clients.removeOne(client);
        client->deleteLater();
    }
}
//! [clientDisconnected]

//! [Connect to remote service]
void Chat::connectClicked()
{
    ui->connectButton->setEnabled(false);
    ui->disconnButton->setEnabled(false);
    ui->sendButton->setEnabled(false);

    // scan for services
    /*const QBluetoothAddress adapter = localAdapters.isEmpty() ?
                                           QBluetoothAddress() :
                                           localAdapters.at(currentAdapterIndex).address();

    RemoteSelector remoteSelector(adapter);*/
    if(m_remoteSelector)
    {
        delete m_remoteSelector;
    }

    m_remoteSelector = new RemoteSelector(m_adapter);
    if(nullptr == m_remoteSelector)
    {
        QMessageBox::critical(nullptr, "Error!", "remoteSelector NULL");
        ui->connectButton->setEnabled(true);
        return;
    }
#ifdef Q_OS_ANDROID
    if (QtAndroid::androidSdkVersion() >= 23)
        //remoteSelector.startDiscovery(QBluetoothUuid(reverseUuid));
        m_remoteSelector->startDiscovery(QBluetoothUuid(reverseUuid));
    else
        //remoteSelector.startDiscovery(QBluetoothUuid(serviceUuid));
        m_remoteSelector->startDiscovery(QBluetoothUuid(serviceUuid));
#else
    //remoteSelector.startDiscovery(QBluetoothUuid(serviceUuid));
    m_remoteSelector->startDiscovery(QBluetoothUuid(serviceUuid));
#endif
    //if (remoteSelector.exec() == QDialog::Accepted) {
    if (m_remoteSelector->exec() == QDialog::Accepted) {
        //QBluetoothServiceInfo service = remoteSelector.service();
        //remoteSelector.scan_char();
        m_service = m_remoteSelector->intersted_service();
        m_rx_char = m_remoteSelector->rx_char();
        m_tx_char = m_remoteSelector->tx_char();
        Q_ASSERT(m_service && m_rx_char && m_tx_char);
        /* qDebug() << "Characterstic:\n"
               << m_rx_char->getName() << "," << m_rx_char->getUuid() << "\n"
               << m_tx_char->getName() << "," << m_tx_char->getUuid(); */
        connect(m_service->service(), &QLowEnergyService::characteristicWritten,
                this, &Chat::BleServiceCharacteristicWrite);
        connect(m_service->service(), &QLowEnergyService::characteristicRead,
                this, &Chat::BleServiceCharacteristicRead);
        connect(m_service->service(), &QLowEnergyService::characteristicChanged,
                this, &Chat::BleServiceCharacteristicChanged);

        const QLowEnergyDescriptor &rx_descp
                //= m_rx_char->getCharacteristic().descriptors()[0];
                = m_rx_char->getCharacteristic().descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
        m_service->service()->writeDescriptor(rx_descp, QByteArray::fromHex("0100"));
        /*
        ServiceInfo * service = remoteSelector.intersted_service();
        qDebug() << "Connecting to service 2" << service->getName()
                 << "on" << remoteSelector.current_device()->name();//service.device().name();
        // Create client
        qDebug() << "Going to create client";
        ChatClient *client = new ChatClient(this);
qDebug() << "Connecting...";

        connect(client, &ChatClient::messageReceived,
                this, &Chat::showMessage);
        connect(client, &ChatClient::disconnected,
                this, QOverload<>::of(&Chat::clientDisconnected));
        connect(client, QOverload<const QString &>::of(&ChatClient::connected),
                this, &Chat::connected);
        connect(client, &ChatClient::socketErrorOccurred,
                this, &Chat::reactOnSocketError);
        connect(this, &Chat::sendMessage, client, &ChatClient::sendMessage);
qDebug() << "Start client";
        client->startClient(service);

        clients.append(client);
        */
        ui->connectButton->setEnabled(false);
        ui->disconnButton->setEnabled(true);
        ui->sendButton->setEnabled(true);
    }
    else
    {
        ui->connectButton->setEnabled(true);
    }
}
//! [Connect to remote service]

//! [sendClicked]
void Chat::sendClicked()
{
    if(m_tx_char->getCharacteristic().isValid())
    {
        if(!m_dir_ready)
        {
            bool mkdir_ret = false;
            QDir data_dir(m_data_file_pth_str);
            mkdir_ret = data_dir.mkpath(m_redundant_dir_rel_name);

            if(!mkdir_ret)
            {
                QMessageBox::critical(nullptr, "!!!",
                                      "Make dir \"" + m_redundant_file_path_str
                                      + "\" fail!");
                return;
            }
            m_dir_ready = true;
        }
        if(m_file_write_ready)
        {
            m_file.close();
            m_valid_data_file.close();
            m_file_write_ready = false;
        }
        QString dtms_str = diy_curr_date_time_str_ms();
        QString data_file_name
                = m_redundant_file_path_str + "/"
                  + dtms_str + m_file_name_apx + ".txt";
        QString valid_data_file_name
                = m_data_file_pth_str + "/"
                  + dtms_str + ".txt";
        m_file.setFileName(data_file_name);
        m_valid_data_file.setFileName(valid_data_file_name);
        m_file_write_ready = m_file.open(QIODevice::WriteOnly);
        if(m_file_write_ready)
        {
            if(!(m_file_write_ready = m_valid_data_file.open(QIODevice::WriteOnly)))
            {
                m_file.close();
                QMessageBox::critical(nullptr, "!!!",
                                      "Create file \"" + valid_data_file_name
                                      + "\" fail!");
                return;
            }
        }
        else
        {
            QMessageBox::critical(nullptr, "!!!",
                                  "Create file \"" + data_file_name + "\" fail!");
            return;
        }

        qDebug() << "Now write to char" << m_tx_char->getUuid();
        ui->connectButton->setEnabled(false);
        ui->disconnButton->setEnabled(false);
        ui->sendButton->setEnabled(false);
        for(int i=0; i < m_light_num; i++)
        {
            //read_notify();
            m_service->service()->writeCharacteristic(m_tx_char->getCharacteristic(),
                                                      m_write_data_list[i]);
            //read_notify();
            QThread::msleep(500);
        }
    }
    else
    {
        QMessageBox::question(nullptr, "!!!", "No valid characteristic!");
    }
/*
    ui->sendButton->setEnabled(false);
    ui->sendText->setEnabled(false);

    showMessage(localName, ui->sendText->text());
    emit sendMessage(ui->sendText->text());

    ui->sendText->clear();

    ui->sendText->setEnabled(true);
    ui->sendButton->setEnabled(true);
*/
}
//! [sendClicked]
void Chat::BleServiceCharacteristicWrite(const QLowEnergyCharacteristic &c,
                                         const QByteArray &value)
{
/*    qDebug() << "Write the " << (int)value[m_light_idx_pos]
                << " OK:" << QByteHexString(value);
*/
}


//! [showMessage]
void Chat::showMessage(const QString &sender, const QString &message)
{
    ui->chat->insertPlainText(QString::fromLatin1("%1: %2\n").arg(sender, message));
    ui->chat->ensureCursorVisible();
}
//! [showMessage]

void Chat::read_notify()
{
    if(m_service)
    {
        m_service->service()->readCharacteristic(m_rx_char->getCharacteristic());
    }
}
void Chat::BleServiceCharacteristicRead(const QLowEnergyCharacteristic &c,
                                        const QByteArray &value)
{
    QString read_data_str;
    read_data_str = QByteHexString(value);
    qDebug() << "Read data:" << read_data_str;
}

void Chat::BleServiceCharacteristicChanged(const QLowEnergyCharacteristic &c,
                                        const QByteArray &value)
{
    QString str, utf8_str;
    str = QByteHexString(value);

    qDebug() << "Char Changed:" << str;
    if(m_file_write_ready)
    {
        utf8_str = str + "\r\n";
        m_file.write(utf8_str.toUtf8());
        if(value.startsWith(m_valid_data_flag))
        {
            m_valid_data_file.write(utf8_str.toUtf8());
            ui->chat->insertPlainText(utf8_str);
        }
    }
    if(value.startsWith(m_valid_data_flag)
            && ((unsigned char)value[m_light_idx_pos]) == (unsigned char)m_light_num)
    {
        m_file.flush();
        m_valid_data_file.flush();
        write_data_done_notify();
    }
}

void Chat::write_data_done_notify()
{
    emit write_data_done_sig();
}

void Chat::write_data_done_handle()
{
    QMessageBox::information(nullptr, "OK", "数据采集完成");
    ui->connectButton->setEnabled(false);
    ui->disconnButton->setEnabled(true);
    ui->sendButton->setEnabled(true);
    /*if(m_file_write_ready)
    {
        m_file.close();
        m_file_write_ready = false;
    }*/
}

void Chat::on_chat_textChanged()
{
    ui->chat->moveCursor(QTextCursor::End);
}

void Chat::restart_work()
{
    delete m_remoteSelector;
    m_remoteSelector = nullptr;
    m_service = nullptr;
    m_rx_char = nullptr;
    m_tx_char = nullptr;

    ui->sendButton->setEnabled(false);
    ui->connectButton->setEnabled(true);
    ui->disconnButton->setEnabled(false);
}
void Chat::on_disconnButton_clicked()
{
    restart_work();

    if(m_file_write_ready)
    {
        m_file.close();
        m_valid_data_file.close();
        m_file_write_ready = false;
    }
}

void Chat::init_write_data()
{
    unsigned char write_data[m_light_num][m_write_data_len]=
    {
        {//1
0x3D,0x11,0x1,0x23,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x99
        },
        {//2
0x3D,0x11,0x2,0x23,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x8B
        },
        {//3
0x3D,0x11,0x3,0x28,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x93
        },
        {//4
0x3D,0x11,0x4,0x64,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x21
        },
        {//5
0x3D,0x11,0x5,0x2D,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xBD
        },
        {//6
0x3D,0x11,0x6,0x23,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xB3
        },
        {//7
0x3D,0x11,0x7,0x50,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x5B
        },
        {//8
0x3D,0x11,0x8,0x2D,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xFB
        },
        {//9
0x3D,0x11,0x9,0x28,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xFF
        },
        {//10
0x3D,0x11,0xA,0x28,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xED
        },
        {//11
0x3D,0x11,0xB,0x3C,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xCB
        },
        {//12
0x3D,0x11,0xC,0x2D,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xC3
        },
        {//13
0x3D,0x11,0xD,0xA,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x83
        },
        {//14
0x3D,0x11,0xE,0x23,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xC3
        },
        {//15
0x3D,0x11,0xF,0x55,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x21
        },
        {//16
0x3D,0x11,0x10,0x53,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x97
        },
        {//17
0x3D,0x11,0x11,0x55,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x95
        },
        {//18
0x3D,0x11,0x12,0x5A,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x99
        },
        {//19
0x3D,0x11,0x13,0x5A,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x97
        },
    };
    int i;
    for(i=0; i< m_light_num; i++)
    {
        m_write_data_list.append(QByteArray((const char*)write_data[i], m_write_data_len));
    }
}
