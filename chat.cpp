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

#include <QtCore/qdebug.h>

#include <QtBluetooth/qbluetoothdeviceinfo.h>
#include <QtBluetooth/qbluetoothlocaldevice.h>
#include <QtBluetooth/qbluetoothuuid.h>
#include <QMessageBox>

#include "types_and_defs.h"
#include "logger.h"
#include "ble_comm_pkt.h"

#ifdef Q_OS_ANDROID
#include <QtAndroidExtras/QtAndroid>
#endif

#ifdef Q_OS_ANDROID
static const QLatin1String reverseUuid("c8e96402-0102-cf9c-274b-701a950fe1e8");
#endif

#if (QT_VERSION < QT_VERSION_CHECK(6,0,0))
#define BLE_CLIENT_CHAR_CFG (QBluetoothUuid::ClientCharacteristicConfiguration)
#else
#define BLE_CLIENT_CHAR_CFG (QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration)
#endif

Chat::Chat(QWidget *parent)
    : QDialog(parent), ui(new Ui_Chat)
{
    start_log_thread(m_log_thread);

    DIY_LOG(LOG_LEVEL::LOG_INFO, "+++++++++++++Chat construct in thread: %u",
            (quint64)(QThread::currentThreadId()));

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

    //! [Get local device name]
    localName = QBluetoothLocalDevice().name();
    //! [Get local device name]

    //try to load settings from xml.
    load_sw_settings(m_sw_settings);
    //new db management object.
    //chat only manages m_skin_db, and the latter manages remote db and thread.
    if(m_sw_settings.oth_settings.use_remote_db)
    {
        m_skin_db = new SkinDatabase(&m_sw_settings.db_info);
    }
    else
    {
        m_skin_db = new SkinDatabase(nullptr);
    }

    if(!m_skin_db)
    {
        DIY_LOG(LOG_LEVEL::LOG_ERROR,
                "New SkinDatabase error, so all db and csv store can't be taken.");
    }
    else if(m_sw_settings.oth_settings.use_remote_db)
    {
        m_skin_db->set_safe_ldb_for_rdb_fpn(QString("../") + m_data_dir_rel_name
                                            + "/" + m_safe_ldb_for_rdb_dir_rel_name,
                                            m_safe_ldb_for_rdb_rel_name);
        connect(m_skin_db, &SkinDatabase::rdb_state_upd_sig,
                this,  &Chat::rdb_state_upd_handler);
        connect(m_skin_db, &SkinDatabase::upload_safe_ldb_end_sig,
                this,  &Chat::upload_safe_ldb_end_handler);
    }

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
    m_local_db_pth_str = m_data_pth_str + "/" + QString(m_local_db_dir_rel_name);
    if(m_skin_db)
    {
        m_skin_db->set_local_store_pth_str(m_local_db_pth_str, m_data_pth_str);
    }

    m_adapter = localAdapters.isEmpty() ? QBluetoothAddress() :
                           localAdapters.at(currentAdapterIndex).address();

    ui->sendButton->setEnabled(false);
    ui->manualCmdBtn->setEnabled(false);
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
    m_db_data.obj_id = "";
    m_db_data.skin_type = "";
    m_db_data.pos = "";
    m_db_data.obj_desc = "";
    m_db_data.expt_id = "";
    m_db_data.expt_date = QDate::currentDate().toString("yyyy-MM-dd");
    m_db_data.expt_time = QTime::currentTime().toString("HH:mm:ss");
    m_db_data.expt_desc = "";
    m_db_data.dev_id = "";
    m_db_data.dev_addr = "";
    m_db_data.dev_desc = "";
    m_db_data.expt_changed = false;
    m_db_data.obj_changed = false;
    m_db_data.dev_changed = false;
    ui->currFileradioButton->setChecked(true);
    ui->currFolderradioButton->setChecked(false);

    ui->skinTypeComboBox->addItems(g_skin_type);
    ui->posComboBox->addItems(g_sample_pos);
}

Chat::~Chat()
{
    DIY_LOG(LOG_LEVEL::LOG_INFO, "----------Chat destructor in thread: %u",
            (quint64)(QThread::currentThreadId()));

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
    if(m_skin_db)
    {
        delete m_skin_db;
    }

    end_log_thread(m_log_thread);
}

void Chat::clientConnected(const QString &name)
{
    ui->chat->insertPlainText(QString::fromLatin1("%1 has joined chat.\n").arg(name));
}

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

//! [Connect to remote service]
void Chat::connectClicked()
{

    ui->connectButton->setEnabled(false);
    ui->disconnButton->setEnabled(false);
    ui->sendButton->setEnabled(false);
    ui->manualCmdBtn->setEnabled(false);
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
            err = QString("Device service info error:\n")
                + QString("m_service:0x\n") + QString::number((quint64)m_service,16)
                + QString("m_rx_char:0x\n") + QString::number((quint64)m_rx_char,16)
                + QString("m_tx_char:0x\n") + QString::number((quint64)m_tx_char,16)
                + QString("m_work_dev_info:0x\n") + QString::number((quint64)m_work_dev_info,16);
            QMessageBox::critical(nullptr, "!!!", err);
            DIY_LOG(LOG_LEVEL::LOG_ERROR, "%ls", err.utf16());
            return;
        }

        m_curr_light_no = m_work_dev_info->light_list.begin();
        m_db_data.dev_addr = m_work_dev_info->addr;

        connect(m_service->service(), &QLowEnergyService::characteristicWritten,
                this, &Chat::BleServiceCharacteristicWritten);
        connect(m_service->service(), &QLowEnergyService::characteristicRead,
                this, &Chat::BleServiceCharacteristicRead);
        connect(m_service->service(), &QLowEnergyService::characteristicChanged,
                this, &Chat::BleServiceCharacteristicChanged);

         connect(m_service->service(),
        #if (QT_VERSION < QT_VERSION_CHECK(6,2,0))
                 QOverload<QLowEnergyService::ServiceError>::of(&QLowEnergyService::error),
        #else
                 &QLowEnergyService::errorOccurred,
        #endif
                 this,
                 &Chat::BleServiceError);

        const QLowEnergyDescriptor &rx_descp
                //= m_rx_char->getCharacteristic().descriptors()[0];
                = m_rx_char->getCharacteristic().descriptor(BLE_CLIENT_CHAR_CFG);
        m_service->service()->writeDescriptor(rx_descp, QByteArray::fromHex("0100"));
        ui->connectButton->setEnabled(false);
        ui->disconnButton->setEnabled(true);
        ui->sendButton->setEnabled(true);
        set_manual_cmd_btn();
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
    start_send_data_to_device(false);
}

void Chat::on_calibrationButton_clicked()
{
    m_calibrating = true;
    QMessageBox::information(nullptr, "提示：", "请将设备放置在无遮挡的位置进行校准");
    start_send_data_to_device(false);
}

/*
 * Return:
 *      true: go on.
 *      false: stop.
*/
bool Chat::check_volu_info()
{
    if(m_calibrating)
    {
        return true;
    }

    QString pos_str, skin_type_str;
    pos_str = ui->posComboBox->currentText();
    skin_type_str = ui->skinTypeComboBox->currentText();
    if(pos_str.isEmpty() || skin_type_str.isEmpty())
    {

        QString err = QString("%1和%2不能为空！").arg(ui->posLabel->text(),
                                                ui->skinTypeLabel->text());
        DIY_LOG(LOG_LEVEL::LOG_ERROR, "%ls", err.utf16());
        QMessageBox::critical(nullptr, "!!!", err);
        return false;
    }

    QString curr_no, err_str = "";
    curr_no = ui->numberTextEdit->text();
    if(curr_no.isEmpty())
    {
        err_str = "编号为空，请确认是否继续！";
    }
    else if(m_db_data.obj_id == curr_no)
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
        //{m_csv_pth_str, true}, //do not use this now.
        {m_all_rec_pth_str, !m_only_rec_valid_data},
        {m_local_db_pth_str, true},
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
    QString now_pos, now_obj_id;
    if(m_calibrating)
    {
        now_pos = m_calibration_pos;
        now_obj_id = m_calibration_no;
    }
    else
    {
        now_pos = ui->posComboBox->currentText();
        now_obj_id = ui->numberTextEdit->text();
    }
    if(m_first_check || (now_obj_id != m_db_data.obj_id)
        || (ui->skinTypeComboBox->currentText()) != (m_db_data.skin_type)
        || (ui->objDescTextEdit->text()) != (m_db_data.obj_desc))
    {
        m_db_data.obj_changed = true;
        m_db_data.obj_id = now_obj_id;
        m_db_data.skin_type = ui->skinTypeComboBox->currentText();
        m_db_data.obj_desc = ui->objDescTextEdit->text();
    }
    else
    {
        m_db_data.obj_changed = false;
    }
    m_db_data.pos = now_pos;

    if(m_first_check || (ui->devIDTextEdit->text() != m_db_data.dev_id)
       || (ui->devDescTextEdit->text() != m_db_data.dev_desc))
    {
        m_db_data.dev_changed = true;
        m_db_data.dev_id = ui->devIDTextEdit->text();
        m_db_data.dev_desc = ui->devDescTextEdit->text();
    }
    else
    {
        m_db_data.dev_changed = false;
    }
    if(m_first_check || (ui->exptIDTextEdit->text() != m_db_data.expt_id)
        || (ui->exptDescTextEdit->text() != m_db_data.expt_desc))
    {
        m_db_data.expt_changed = true;
        m_db_data.expt_id = ui->exptIDTextEdit->text();
        m_db_data.expt_desc = ui->exptDescTextEdit->text();
    }
    else
    {
        m_db_data.expt_changed = false;
    }

    QString s_info_str = m_db_data.obj_id + "_" + m_db_data.pos + "---";
    QString dtms_str = diy_curr_date_time_str_ms();
    m_curr_file_bn_str =  s_info_str + dtms_str;
    QString all_rec_pf_str = m_all_rec_pth_str + "/"
                             + m_curr_file_bn_str + m_all_rec_file_name_apx;
    QString txt_file_pf_str = m_txt_pth_str + "/" + m_curr_file_bn_str;
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
    m_db_data.rec_date = QDate::currentDate().toString("yyyy-MM-dd");
    m_db_data.rec_time = QTime::currentTime().toString("HH:mm:ss");
    return true;
}

void Chat::start_send_data_to_device(bool single_cmd)
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

    if(m_tx_char && m_tx_char->getCharacteristic().isValid())
    {
        if(!check_volu_info())
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
        ui->manualCmdBtn->setEnabled(false);
        ui->calibrationButton->setEnabled(false);

        QByteArray bytes_to_send;
        bool gen_pkt_ok;
        //if (!m_calibrating && ui->sendText->text().length() >0)
        m_db_data.lambda_data.clear();
        if(single_cmd)
        {
            m_single_light_write = true;
            QString send_txt = ui->sendText->text();
            send_txt.remove(' ');
            bytes_to_send = QByteArray::fromHex(send_txt.toUtf8());
            quint8 data_crc8 = diy_crc8_8210(bytes_to_send, bytes_to_send.count());
            bytes_to_send.append(data_crc8);
            gen_pkt_ok = true;
        }
        else
        {
            m_single_light_write = false;
            m_curr_light_no = m_work_dev_info->light_list.begin();
            gen_pkt_ok = ble_comm_gen_app_light_pkt(bytes_to_send,
                                       m_curr_light_no.value()->idx, m_work_dev_info->dev_type);
        }

        if(gen_pkt_ok)
        {
            DIY_LOG(LOG_LEVEL::LOG_INFO,
                    "<<<<<<<<Now write Char %ls, %ls\n",
                    m_tx_char->getUuid().utf16(),
                    QByteHexString(bytes_to_send).utf16());
            m_service->service()->writeCharacteristic(m_tx_char->getCharacteristic(),
                                                      bytes_to_send);
            DIY_LOG(LOG_LEVEL::LOG_INFO, "Start write wait timer...");
            m_write_wait_resp_timer.start(m_work_dev_info->single_light_wait_time);
            DIY_LOG(LOG_LEVEL::LOG_INFO, "\tWrite end.");
        }
        else
        {
            DIY_LOG(LOG_LEVEL::LOG_ERROR, "Generate packet error!");
            QMessageBox::critical(nullptr, "!!!", "Generate packet error!");
            return;
        }
    }
    else
    {
        DIY_LOG(LOG_LEVEL::LOG_INFO,"No valid characteristic!");
        QMessageBox::critical(nullptr, "!!!", "No valid characteristic!");
    }
}

//! [showMessage]
void Chat::showMessage(const QString &sender, const QString &message)
{
    ui->chat->insertPlainText(QString::fromLatin1("%1: %2\n").arg(sender, message));
    ui->chat->ensureCursorVisible();
}
//! [showMessage]

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
    QByteArray data
            = QByteHexString(QByteArray(&(value.constData()[ORID_DATA_POS_START]),
                                 ORID_DATA_POS_LEN), "").toUtf8();

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
        SkinDatabase::db_lambda_data_s_t ld;
        ld.lambda = m_curr_light_no.value()->lambda;
        ld.data = data.toULongLong(nullptr, 16);
        m_db_data.lambda_data.append(ld);
        val_p_prf.prepend("***");
        ++m_curr_light_no;
    }
    else if(value.startsWith(ORID_DEV_FINISH_PKT_HDR))
    {
        if(m_write_wait_resp_timer.isActive())
        {
            m_write_wait_resp_timer.stop();
        }
        if(!m_single_light_write && (m_curr_light_no != m_work_dev_info->light_list.end()))
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
            m_curr_light_no = m_work_dev_info->light_list.begin();
            m_write_done_timer.start(500);
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

void Chat::turn_on_next_light(light_list_t::Iterator no)
{
    if(no != m_work_dev_info->light_list.end())
    {
        QByteArray bytes_to_send;
        bool gen_pkt_ok =  ble_comm_gen_app_light_pkt(bytes_to_send,
                                                      m_curr_light_no.value()->idx,
                                                      m_work_dev_info->dev_type);
        if(gen_pkt_ok)
        {
            DIY_LOG(LOG_LEVEL::LOG_INFO,
                    "<<<<<<<<Now write Char %ls, %ls\n",
                    m_tx_char->getUuid().utf16(),
                    QByteHexString(bytes_to_send).utf16());
            m_service->service()->writeCharacteristic(m_tx_char->getCharacteristic(),
                                                      bytes_to_send);
            m_write_wait_resp_timer.start(m_work_dev_info->single_light_wait_time);
            DIY_LOG(LOG_LEVEL::LOG_INFO, "\tWrite end.");
        }
        else
        {
            DIY_LOG(LOG_LEVEL::LOG_ERROR, "Generate packet error!");
            QMessageBox::critical(nullptr, "!!!", "Generate packet error!");
        }
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
    bool non_empty = true;
    if(done)
    {
        err = "数据采集完成";
        title = "OK";
    }
    else
    {
        if(m_single_light_write)
        {
            non_empty = false;
            err = QString("超时，未收到数据。结束！");
        }
        else
        {
            if(m_curr_light_no == m_work_dev_info->light_list.begin())
            {
                non_empty = false;
            }
            err = QString("超时，未收到波长=%1，idx=%2的灯光数据。结束！").\
                    arg(m_curr_light_no.value()->lambda).\
                    arg(m_curr_light_no.value()->idx);
        }
        title = "!!!";
    }
    ui->connectButton->setEnabled(false);
    ui->disconnButton->setEnabled(true);
    ui->sendButton->setEnabled(true);
    set_manual_cmd_btn();
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
    if(non_empty && m_skin_db)
    {
        if(m_skin_db->store_these_info(m_db_data))
        {
            m_first_check = false;
        }
    }
    DIY_LOG(LOG_LEVEL::LOG_INFO, "%ls", err.utf16());
    QMessageBox::information(nullptr, title, err);
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
    set_manual_cmd_btn();
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

/*no use now. we generate data packet dynamicly
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
*/

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
        m_local_db_pth_str = m_data_pth_str + "/" + QString(m_local_db_dir_rel_name);
        m_dir_ready = false;

        if(m_skin_db)
        {
            m_skin_db->set_local_store_pth_str(m_local_db_pth_str, m_data_pth_str);
        }
        if(m_sw_settings.oth_settings.use_remote_db && m_skin_db)
        {
            m_skin_db->set_safe_ldb_for_rdb_fpn(m_data_pth_str + "/"
                                              + m_safe_ldb_for_rdb_dir_rel_name,
                                                m_safe_ldb_for_rdb_rel_name);

        }
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
    QString file_pos;
    if(ui->currFileradioButton->isChecked())
    {
        file_pos = m_data_pth_str + "/" + QString(m_txt_dir_rel_name) + "/"
                + m_curr_file_bn_str + m_txt_ext;
        QFile f(file_pos);
        if(!f.exists())
        {
            QMessageBox::warning(nullptr, "!!!", "尚未采集数据！");
            return;
        }
    }
    else if(ui->otherFileradioButton->isChecked())
    {
        QString fpn = QFileDialog::getOpenFileName(this, tr("请选择文件"),
                                                 QDir::currentPath(),
                                                 tr("Text Files(*.txt)"));
        if(fpn.isEmpty())
        {
            return;
        }
        else
        {
            file_pos = fpn;
            ui->currFileNameLabel->setText(QFileInfo(fpn).fileName());
        }
    }
    else if(ui->currFolderradioButton->isChecked())
    {
        file_pos = m_data_pth_str + "/" + QString(m_txt_dir_rel_name);
    }
    else
    {
        QMessageBox::warning(nullptr, "!!!", "请选择显示文件还是文件夹内容");
        return;
    }
    QStringList parms;
    parms.append(file_pos);

    QString cmd_exe1 = QString("./") + QString(m_visual_exe_fpn);
    QFile cmd_exe_f(cmd_exe1);
    if(!cmd_exe_f.exists())
    {
        QString cmd_exe2 = QString("../") + QString(m_visual_exe_fpn);
        cmd_exe_f.setFileName(cmd_exe2);
        if(!cmd_exe_f.exists())
        {
            QString err;
            err = QString("未找到可视化执行程序！请确认位置：\n")
                    + QString(cmd_exe1) + QString("\n或\n") + QString(cmd_exe2);
            QMessageBox::warning(nullptr, "!!!", err);
            return;
        }
    }
    qDebug() << "cmd line:" << cmd_exe_f.fileName();//m_visual_exe_fpn;
    for(auto &s : parms)
    {
        qDebug() << s;
    }
    if(QProcess::startDetached(cmd_exe_f.fileName(), parms))
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

void Chat::on_manualCmdBtn_clicked()
{
    if(ui->sendText->text().length() <= 0)
    {
        QMessageBox::critical(nullptr, "!!!", "请输入命令（十六进制字符串，可以包含空格）!");
        return;
    }
    start_send_data_to_device(true);
}

void Chat::set_manual_cmd_btn()
{
    if(ui->sendText->text().isEmpty())
    {
        ui->manualCmdBtn->setEnabled(false);
    }
    else
    {
        ui->manualCmdBtn->setEnabled(true);
    }
}

void Chat::on_sendText_textEdited(const QString &/*arg1*/)
{
    if(ui->sendButton->isEnabled())
    {
        set_manual_cmd_btn();
    }
}

void Chat::on_quitButton_clicked()
{
    if(m_skin_db)
    {
        m_skin_db->close_dbs(SkinDatabase::DB_ALL);
    }
}

void Chat::closeEvent(QCloseEvent *event)
{
    if(m_skin_db)
    {
        m_skin_db->close_dbs(SkinDatabase::DB_ALL);
    }
    event->accept();
}

void Chat::upload_safe_ldb_end_handler(QList<SkinDatabase::tbl_rec_op_result_t> op_result,
                                       bool result_ret)
{
    m_remote_db_wait_box.hide();

    m_upload_safe_ldb_now = false;
    QString result_str = QString("Upload result:\n");
    int fail_cnt = 0;

    foreach(const SkinDatabase::tbl_rec_op_result_t &tbl_ret, op_result)
    {
        QString s = tbl_ret.tbl_name +
                QString(": succ:%1, fail:%2, partialy-succ:%3. Total %4.\n")
                .arg(tbl_ret.rec_succ).arg(tbl_ret.rec_fail)
                .arg(tbl_ret.rec_part_succ).arg(tbl_ret.rec_cnt);
        result_str += s;
        fail_cnt += tbl_ret.rec_fail;
    }
    if(0 == fail_cnt)
    {
        result_str += "All records in safe ldb has been upload to remote db!\n";
        if(result_ret)
        {
            result_str += "And the safe ldb file has been deleted.\n";
        }
        else
        {
            result_str += "But fails to delete the safe ldb file. PLease delete it manually.\n";
        }
    }
    if(result_ret)
    {
        result_str += "Not all records are uploaded to remote db. Please check log file.\n";
    }
    QMessageBox::information(nullptr, "Result", result_str);
}

void Chat::select_safe_ldb_for_upload()
{
    QString fpn = QFileDialog::getOpenFileName(this, tr("请选择文件"),
                                             QDir::currentPath(), tr("(*.sqlite)"));
    if(fpn.isEmpty())
    {
        return;
    }
    else
    {
        ui->safeldbLabel->setText(QFileInfo(fpn).fileName());

        m_safe_ldb_for_upload_fpn = fpn;
        m_skin_db->upload_safe_ldb(m_safe_ldb_for_upload_fpn);

        m_remote_db_wait_box.setText("Uploading...");
        m_remote_db_wait_box.setStandardButtons(QMessageBox::NoButton);
        m_remote_db_wait_box.setWindowFlags(Qt::FramelessWindowHint | Qt::CustomizeWindowHint);
        m_remote_db_wait_box.setWindowModality(Qt::NonModal);
        m_remote_db_wait_box.show();
    }
}

void Chat::rdb_state_upd_handler(SkinDatabase::rdb_state_t rdb_st)
{
    m_remote_db_wait_box.hide();
    if(m_upload_safe_ldb_now)
    {
        if(rdb_st == SkinDatabase::RDB_ST_OK)
        {
            select_safe_ldb_for_upload();
        }
        else
        {
            QMessageBox::critical(nullptr, "!!!",
                                  "Remote db is not availabel");
        }
    }
}
void Chat::on_uploadLdbPB_clicked()
{
    if(!m_skin_db)
    {
        QMessageBox::critical(nullptr, "!!!",
                              "All db function can't work. Please check the log.");
        return;
    }
    m_upload_safe_ldb_now = true;
    if(m_skin_db->remote_db_st() == SkinDatabase::RDB_ST_OK)
    {
        select_safe_ldb_for_upload();
    }
    else
    {
        m_skin_db->prepare_remote_db();

        m_remote_db_wait_box.setText("Remote db preparing...");
        m_remote_db_wait_box.setStandardButtons(QMessageBox::NoButton);
        m_remote_db_wait_box.setWindowFlags(Qt::FramelessWindowHint | Qt::CustomizeWindowHint);
        m_remote_db_wait_box.setWindowModality(Qt::NonModal);
        m_remote_db_wait_box.show();
    }
}
