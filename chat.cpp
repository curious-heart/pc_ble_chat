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

#include "types_and_defs.h"
#include "logger.h"
#include "ble_comm_pkt.h"

#ifdef Q_OS_ANDROID
#include <QtAndroidExtras/QtAndroid>
#endif

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

    //try to load settings from xml.
    load_sw_settings(m_sw_settings);

    init_write_data();

    connect(&m_write_wait_resp_timer, &QTimer::timeout,
           this, &Chat::write_wait_resp_timeout);
    m_write_wait_resp_timer.setSingleShot(true);
    connect(this, &Chat::turn_on_next_light_sig,
           this, &Chat::turn_on_next_light);

    connect(&m_write_done_timer, &QTimer::timeout,
           this, &Chat::write_data_done_notify);
    m_write_done_timer.setSingleShot(true);
    connect(this, &Chat::write_data_done_sig,
            this, &Chat::write_data_done_handle);

    m_data_pth_str = "../" + QString(m_data_dir_rel_name);
    m_all_rec_pth_str= m_data_pth_str + "/" + QString(m_all_rec_dir_rel_name);
    m_txt_pth_str = m_data_pth_str  + "/" + QString(m_txt_dir_rel_name);
    m_csv_pth_str = m_data_pth_str  + "/" + QString(m_csv_dir_rel_name);

    m_adapter = localAdapters.isEmpty() ? QBluetoothAddress() :
                           localAdapters.at(currentAdapterIndex).address();

    ui->sendButton->setEnabled(false);
    ui->calibrationButton->setEnabled(false);
    ui->disconnButton->setEnabled(false);
    if(m_all_dev_scan)
    {
        ui->allDevcheckBox->setCheckState(Qt::Checked);
    }
    else
    {
        ui->allDevcheckBox->setCheckState(Qt::Unchecked);
    }

    ui->storePathDisplay->setText(m_data_pth_str);

    if(m_only_rec_valid_data)
    {
        ui->onlyValidDatacheckBox->setCheckState(Qt::Checked);
    }
    else
    {
        ui->onlyValidDatacheckBox->setCheckState(Qt::Unchecked);
    }
    m_data_no = "";// ui->numberTextEdit->toPlainText();
    ui->currFileradioButton->setChecked(true);
    ui->currFolderradioButton->setChecked(false);

    ui->skinTypeComboBox->addItems(g_skin_type);
    ui->posComboBox->addItems(g_sample_pos);
}

Chat::~Chat()
{
    qDeleteAll(clients);
    delete server;
    delete m_remoteSelector;

    if(m_file_write_ready)
    {
        if(!m_only_rec_valid_data)
        {
            m_all_rec_file.close();
        }
        m_txt_file.close();
        m_file_write_ready = false;
    }
    clear_loaded_settings(m_sw_settings);
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
    ui->calibrationButton->setEnabled(false);

    // scan for services
    /*const QBluetoothAddress adapter = localAdapters.isEmpty() ?
                                           QBluetoothAddress() :
                                           localAdapters.at(currentAdapterIndex).address();

    RemoteSelector remoteSelector(adapter);*/
    if(m_remoteSelector)
    {
        delete m_remoteSelector;
    }

    m_remoteSelector = new RemoteSelector(m_adapter, m_sw_settings.ble_dev_list);
    if(nullptr == m_remoteSelector)
    {
        QString err = QString("remoteSelector NULL");
        QMessageBox::critical(nullptr, "Error!", err);
        DIY_LOG(LOG_LEVEL::LOG_ERROR, "%ls", err.utf16());
        ui->connectButton->setEnabled(true);
        return;
    }
    m_remoteSelector->search_all_dev(m_all_dev_scan);
#ifdef Q_OS_ANDROID
    static const QLatin1String serviceUuid("e8e10f95-1a70-4b27-9ccf-02010264e9c8");
    if (QtAndroid::androidSdkVersion() >= 23)
        m_remoteSelector->startDiscovery(QBluetoothUuid(reverseUuid));
    else
        m_remoteSelector->startDiscovery(QBluetoothUuid(serviceUuid));
#else
    m_remoteSelector->startDiscovery();
#endif
    if (m_remoteSelector->exec() == QDialog::Accepted) {
        m_service = m_remoteSelector->intersted_service();
        m_rx_char = m_remoteSelector->rx_char();
        m_tx_char = m_remoteSelector->tx_char();
        m_work_dev_info = m_remoteSelector->m_target_dev_setting_info;
        if(!m_service || !m_rx_char || !m_tx_char || !m_work_dev_info)
        {
            QString err;
            err = QString("Device service info error:\n"
                          "m_service:0x%1\n"
                          "m_rx_char:0x%2\n"
                          "m_tx_char:0x%3\n"
                          "m_work_dev_info:0x%4").arg(QString((quint64)m_service, 16),
                                                   QString((quint64)m_rx_char,16),
                                                   QString((quint64)m_tx_char,16),
                                                   QString((quint64)m_work_dev_info ,16));
            QMessageBox::critical(nullptr, "!!!", err);
            DIY_LOG(LOG_LEVEL::LOG_ERROR, "%ls", err.utf16());
            return;
        }

        m_light_num = m_work_dev_info->light_list.count();

        connect(m_service->service(), &QLowEnergyService::characteristicWritten,
                this, &Chat::BleServiceCharacteristicWritten);
        connect(m_service->service(), &QLowEnergyService::characteristicRead,
                this, &Chat::BleServiceCharacteristicRead);
        connect(m_service->service(), &QLowEnergyService::characteristicChanged,
                this, &Chat::BleServiceCharacteristicChanged);

         connect(m_service->service(),
                 QOverload<QLowEnergyService::ServiceError>::of(&QLowEnergyService::error),
                 this,
                 &Chat::BleServiceError);

        const QLowEnergyDescriptor &rx_descp
                //= m_rx_char->getCharacteristic().descriptors()[0];
                = m_rx_char->getCharacteristic().descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
        m_service->service()->writeDescriptor(rx_descp, QByteArray::fromHex("0100"));
        ui->connectButton->setEnabled(false);
        ui->disconnButton->setEnabled(true);
        ui->sendButton->setEnabled(true);
        ui->calibrationButton->setEnabled(true);
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
    m_calibrating = false;
    start_send_data_to_device();
}

void Chat::on_calibrationButton_clicked()
{
    m_calibrating = true;
    QMessageBox::information(nullptr, "提示：", "请将设备放置在无遮挡的位置进行校准");
    start_send_data_to_device();
}

/*
 * Return:
 *      true: go on.
 *      false: stop.
*/
bool Chat::check_vul_no_dup()
{
    QString curr_no, err_str = "";
    curr_no = ui->numberTextEdit->toPlainText();
    if(curr_no.isEmpty())
    {
        err_str = "编号为空，请确认是否继续！";
    }
    else if(m_data_no == curr_no)
    {
        err_str = "编号没有更新，请确认是否继续！";
    }
    if(!err_str.isEmpty())
    {
        QMessageBox::StandardButton sel;
        sel = QMessageBox::question(nullptr, "？？？", err_str);
        if(QMessageBox::No == sel)
        {
            return false;
        }
    }
    return true;
}

bool Chat::check_and_mkpth()
{
    typedef struct _pth_flag_t
    {
        QString pth_str;
        bool needed;
    }pth_flag_t;

    pth_flag_t pth_str_list[] = {
        {m_data_pth_str, true},
        {m_txt_pth_str, true},
        {m_csv_pth_str, true},
        {m_all_rec_pth_str, !m_only_rec_valid_data},
    };

    for(int i = 0; i < DIY_SIZE_OF_ARRAY(pth_str_list); i++)
    {
        if(pth_str_list[i].needed && !mkpth_if_not_exists(pth_str_list[i].pth_str))
        {
            QString err;
            err = QString("Directory \"%1\" does not exist,"
                          " and fail to make it.").arg(pth_str_list[i].pth_str);
            DIY_LOG(LOG_LEVEL::LOG_ERROR, "%ls", err.utf16());
            QMessageBox::critical(nullptr, "!!!", err);
            return false;
        }
    }
    return true;
}

bool Chat::prepare_qfile_for_start()
{
    m_data_no = ui->numberTextEdit->toPlainText();
    m_sample_pos = ui->posComboBox->currentText();
    m_skin_type = ui->skinTypeComboBox->currentText();

    QString s_info_str = m_data_no + "_" + m_sample_pos + "---";
    QString dtms_str = diy_curr_date_time_str_ms();
    m_curr_file_bn_str =  s_info_str + dtms_str;
    QString all_rec_pf_str = m_all_rec_pth_str + "/"
                             + m_curr_file_bn_str + m_all_rec_file_name_apx;
    QString txt_file_pf_str = m_data_pth_str + "/" + m_curr_file_bn_str;
    if(m_calibrating)
    {
        all_rec_pf_str +=  m_calibration_file_name_apx;
        txt_file_pf_str += m_calibration_file_name_apx;
        m_curr_file_bn_str += m_calibration_file_name_apx;
    }
    all_rec_pf_str += m_txt_ext;
    txt_file_pf_str += m_txt_ext;

    m_file_write_ready = true;
    if(!m_only_rec_valid_data)
    {
        m_all_rec_file.setFileName(all_rec_pf_str);
        m_file_write_ready = m_all_rec_file.open(QIODevice::WriteOnly);
    }
    if(m_file_write_ready)
    {
        m_txt_file.setFileName(txt_file_pf_str);
        if(!(m_file_write_ready = m_txt_file.open(QIODevice::WriteOnly)))
        {
            if(!m_only_rec_valid_data)
            {
                m_all_rec_file.close();
            }
            QString err = QString("Create file \"%1\" fail!").arg(txt_file_pf_str);
            DIY_LOG(LOG_LEVEL::LOG_ERROR, "%ls", err.utf16());
            QMessageBox::critical(nullptr, "!!!", err);
            return false;
        }
    }
    else
    {
        QString err = QString("Create file \"%1\" fail!").arg(all_rec_pf_str);
        DIY_LOG(LOG_LEVEL::LOG_ERROR, "%ls", err.utf16());
        QMessageBox::critical(nullptr, "!!!", err);
        return false;
    }
    return true;
}

void Chat::start_send_data_to_device()
{
    if(m_file_write_ready)
    {
        if(!m_only_rec_valid_data)
        {
            m_all_rec_file.close();
        }
        m_txt_file.close();
        m_file_write_ready = false;
    }

    if(m_tx_char->getCharacteristic().isValid())
    {
        if(!check_vul_no_dup())
        {
            return;
        }

        if(!m_dir_ready)
        {
            m_dir_ready = check_and_mkpth();
            if(!m_dir_ready)
            {
                return;
            }
        }

        m_file_write_ready = prepare_qfile_for_start();
        if(!m_file_write_ready)
        {
            return;
        }

        ui->connectButton->setEnabled(false);
        ui->disconnButton->setEnabled(false);
        ui->sendButton->setEnabled(false);
        ui->calibrationButton->setEnabled(false);

        if (ui->sendText->text().length() >0)
        {
            m_single_light_write = true;
            QString send_txt = ui->sendText->text();
            qDebug() << "Ori txt:" << send_txt;
            send_txt.remove(' ');
            qDebug() << "Space removed:" << send_txt;
            QByteArray send_data = QByteArray::fromHex(send_txt.toUtf8());
            qDebug() << "hex data:" << QByteHexString(send_data);
            quint8 data_crc8 = diy_crc8_8210(send_data, send_data.length());
            send_data.append(data_crc8);
            qDebug() << "hex data with crc:" << QByteHexString(send_data);

            DIY_LOG(LOG_LEVEL::LOG_INFO,
                    "<<<<<<<<Now write Char %ls, %ls\n",
                    m_tx_char->getUuid().utf16(), QString(send_data.toHex(' ')).utf16());
            m_service->service()->writeCharacteristic(m_tx_char->getCharacteristic(),
                                                      send_data);
            m_write_wait_resp_timer.start(m_work_dev_info->single_light_wait_time);
            DIY_LOG(LOG_LEVEL::LOG_INFO, "\tWrite end.");
            //QThread::msleep(500);
        }
        else
        {
            m_single_light_write = false;
            m_curr_light_no = 0;
            DIY_LOG(LOG_LEVEL::LOG_INFO,
                    "<<<<<<<<Now write Char %ls, %ls\n",
                    m_tx_char->getUuid().utf16(),
                    QString(m_write_data_list[m_curr_light_no].toHex(' ')).utf16());
            m_service->service()->writeCharacteristic(m_tx_char->getCharacteristic(),
                                                      m_write_data_list[m_curr_light_no]);
            DIY_LOG(LOG_LEVEL::LOG_INFO, "Start write wait timer...");
            m_write_wait_resp_timer.start(m_work_dev_info->single_light_wait_time);
            DIY_LOG(LOG_LEVEL::LOG_INFO, "\tWrite end.");
            /*
            for(int i=0; i < m_light_num; i++)
            {
                m_service->service()->writeCharacteristic(m_tx_char->getCharacteristic(),
                                                          m_write_data_list[i]);
                QThread::msleep(500);
            }*/
        }
    }
    else
    {
        DIY_LOG(LOG_LEVEL::LOG_INFO,"No valid characteristic!");
        QMessageBox::question(nullptr, "!!!", "No valid characteristic!");
    }
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
    QString s = "Characterstic " + c.uuid().toString() + " read, value: "
            + value.toHex() + "\n";
    DIY_LOG(LOG_LEVEL::LOG_INFO, "%ls", s.utf16());
}

//! [sendClicked]
void Chat::BleServiceCharacteristicWritten(const QLowEnergyCharacteristic &c,
                                         const QByteArray &value)
{
    QString s = ">>>>>>>>Char " + c.uuid().toString() + " written, "
            + value.toHex(' ') + "\n";
    DIY_LOG(LOG_LEVEL::LOG_INFO, "%ls", s.utf16());
/*    qDebug() << "Write the " << (int)value[m_light_idx_pos]
                << " OK:" << QByteHexString(value);
*/
}

void Chat::BleServiceCharacteristicChanged(const QLowEnergyCharacteristic &c,
                                        const QByteArray &value)
{
    QString str, utf8_str;
    QString val_p_prf = " value:";

    str = QByteHexString(value);
    utf8_str = str + "\r\n";
    if(!m_only_rec_valid_data)
    {
        m_all_rec_file.write(utf8_str.toUtf8());
    }
    /*
     * 不对收到的数据内容，做检查。
    */
    if(value.startsWith(ORID_DEV_DATA_PKT_HDR))
    {
        if(m_file_write_ready)
        {
            m_txt_file.write(utf8_str.toUtf8());
            ui->chat->insertPlainText(utf8_str);
        }
        val_p_prf.prepend("***");
        ++m_curr_light_no;
    }
    else if(value.startsWith(ORID_DEV_FINISH_PKT_HDR))
    {
        if(m_write_wait_resp_timer.isActive())
        {
            m_write_wait_resp_timer.stop();
        }
        if((m_curr_light_no < m_light_num) && !m_single_light_write)
        {
            DIY_LOG(LOG_LEVEL::LOG_INFO, "Now turn on next light.");
            emit turn_on_next_light_sig(m_curr_light_no);
        }
        else
        {
            if(!m_only_rec_valid_data)
            {
                m_all_rec_file.flush();
            }
            m_txt_file.flush();
            m_write_done_timer.start(500);
            m_curr_light_no = 0;
        }
    }
    else
    {}

    CharacteristicInfo cInfo(c);
    QString info_s = QString("[====================Char changed:")
                   + QString("(uuid: %1)\n").arg(cInfo.getUuid())
                   + val_p_prf + QString(value.toHex(' ')) + "\n"
                   + QString(" ===================]\n");
    DIY_LOG(LOG_LEVEL::LOG_INFO, "%ls", info_s.utf16());
    //DIY_LOG(LOG_LEVEL::LOG_INFO, "%ls", cInfo.getAllInfoStr().utf16());
}

void Chat::turn_on_next_light(int no)
{
    if(no < m_light_num)
    {
        DIY_LOG(LOG_LEVEL::LOG_INFO,
                "<<<<<<<<Now write Char %ls, %ls\n",
                m_tx_char->getUuid().utf16(),
                QString(m_write_data_list[no].toHex(' ')).utf16());
        m_service->service()->writeCharacteristic(m_tx_char->getCharacteristic(),
                                                  m_write_data_list[no]);
        m_write_wait_resp_timer.start(m_work_dev_info->single_light_wait_time);
        DIY_LOG(LOG_LEVEL::LOG_INFO, "\tWrite end.");
    }
}

void Chat::BleServiceError(QLowEnergyService::ServiceError newError)
{
    /*This string list refers to ServiceError in file qlowenergyservice.h*/
    static const char* ble_service_err_str[]=
    {
        "NoError",
        "OperationError",
        "CharacteristicWriteError",
        "DescriptorWriteError",
        "UnknownError",
        "CharacteristicReadError",
        "DescriptorReadError"
    };
    int idx = (int)newError;
    QString err;
    if((idx >=0) && (idx <DIY_SIZE_OF_ARRAY(ble_service_err_str)))
    {
        err = QString(ble_service_err_str[idx]);
    }
    else
    {
        err = QString("Unknonw Error!");
    }
    DIY_LOG(LOG_LEVEL::LOG_ERROR, "%ls", err.utf16());
}

void Chat::write_wait_resp_timeout()
{
    emit write_data_done_sig(false);
}

void Chat::write_data_done_notify()
{
    emit write_data_done_sig(true);
}

void Chat::write_data_done_handle(bool done)
{
    QString err, title;
    if(done)
    {
        err = "数据采集完成";
        title = "OK";
    }
    else
    {
        if(m_single_light_write)
        {
            err = QString("超时，未收到数据。结束！");
        }
        else
        {
            err = QString("超时，未收到第%1个灯的数据。结束！").arg(m_curr_light_no);
        }
        title = "!!!";
    }
    DIY_LOG(LOG_LEVEL::LOG_INFO, "%ls", err.utf16());
    QMessageBox::information(nullptr, title, err);
    ui->connectButton->setEnabled(false);
    ui->disconnButton->setEnabled(true);
    ui->sendButton->setEnabled(true);
    ui->calibrationButton->setEnabled(true);
    m_write_done_timer.stop();
    if(m_file_write_ready)
    {
        if(!m_only_rec_valid_data)
        {
            m_all_rec_file.close();
        }
        m_txt_file.close();
        m_file_write_ready = false;
        ui->currFileNameLabel->setText(m_curr_file_bn_str);
    }
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
    m_work_dev_info = nullptr;

    ui->sendButton->setEnabled(false);
    ui->calibrationButton->setEnabled(false);
    ui->connectButton->setEnabled(true);
    ui->disconnButton->setEnabled(false);
}
void Chat::on_disconnButton_clicked()
{
    restart_work();

    if(m_file_write_ready)
    {
        if(!m_only_rec_valid_data)
        {
            m_all_rec_file.close();
        }
        m_txt_file.close();
        m_file_write_ready = false;
    }
}

void Chat::init_write_data()
{
    unsigned char write_data[][m_write_data_len]=
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

void Chat::on_clearButton_clicked()
{
    ui->chat->clear();
}

void Chat::on_choosePathButton_clicked()
{
    QString directory = QFileDialog::getExistingDirectory(this,tr("储存位置选择"),QDir::currentPath());
    directory.replace("\\","/"); //双斜杠转换单斜杠
    if(directory.isEmpty())
    {
        QString hint_str;
        hint_str = QString("未选择合适的文件夹，数据文件将存储在下面的位置：")
                + m_data_pth_str;
    }
    else
    {
        qDebug() << "选择完毕！";
        m_data_pth_str = directory;
        m_all_rec_pth_str = m_data_pth_str + "/" + QString(m_all_rec_dir_rel_name);
        m_txt_pth_str = m_data_pth_str  + "/" + QString(m_txt_dir_rel_name);
        m_csv_pth_str = m_data_pth_str  + "/" + QString(m_csv_dir_rel_name);
        m_dir_ready = false;
    }
    ui->storePathDisplay->setText(m_data_pth_str);
    qDebug() << "存储位置：" << directory;
    qDebug() << "m_data_pth_str：" << m_data_pth_str ;
}

void Chat::on_allDevcheckBox_stateChanged(int /*arg1*/)
{
    if(Qt::Checked == ui->allDevcheckBox->checkState())
    {
        m_all_dev_scan = true;
    }
    else
    {
        m_all_dev_scan = false;
    }
}

void Chat::on_onlyValidDatacheckBox_stateChanged(int /*arg1*/)
{
    if(Qt::Checked == ui->onlyValidDatacheckBox->checkState())
    {
        m_only_rec_valid_data = true;
    }
    else
    {
        m_only_rec_valid_data = false;
        m_dir_ready = false;
    }
}


void Chat::on_fileVisualButton_clicked()
{
    if(m_curr_file_bn_str.isEmpty())
    {
        QMessageBox::warning(nullptr, "!!!", "没有可用文件");
        return;
    }

    QString file_pos;
    if(ui->currFileradioButton->isChecked())
    {
        file_pos = m_data_pth_str + "/" + m_curr_file_bn_str + m_txt_ext;
    }
    else if(ui->currFolderradioButton->isChecked())
    {
        file_pos = m_data_pth_str;
    }
    else
    {
        QMessageBox::warning(nullptr, "!!!", "请选择显示文件还是文件夹内容");
        return;
    }
    QStringList parms;
    parms.append(file_pos);

    qDebug() << "cmd line:" << m_visual_exe_fpn;
    for(auto &s : parms)
    {
        qDebug() << s;
    }
    if(QProcess::startDetached(m_visual_exe_fpn, parms))
    {
        QMessageBox::information(nullptr, "OK", "请稍等...");
    }
    else
    {
        QMessageBox::critical(nullptr, "！！！", "显示失败……");
    }
}


void Chat::on_currFileradioButton_clicked()
{
}

void Chat::on_folderradioButton_clicked()
{
}
