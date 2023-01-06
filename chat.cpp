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

#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QLegend>
#include <QBarCategoryAxis>
#include <QValueAxis>

#include "types_and_defs.h"
#include "logger.h"
#include "ble_comm_pkt.h"

#include "chart_display_dlg.h"

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

    //! [Construct UI]
    ui->setupUi(this);

    //connect(ui->quitButton, &QPushButton::clicked, this, &Chat::accept);
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
    bool valid_e;
    load_sw_settings(m_sw_settings, valid_e);
    if(!valid_e)
    {
        QString err = "配置文件内存在非法内容，已启用默认设置。请查看Log确认。";
        ui->stateIndTextEdit->setText(err);
    }
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


        connect(m_skin_db, &SkinDatabase::rdb_write_start_sig,
                this,  &Chat::rdb_write_start_handler);
        connect(m_skin_db, &SkinDatabase::rdb_write_done_sig,
                this,  &Chat::rdb_write_done_hanlder);

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
    disable_data_collection_btns();

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
        ui->uploadLdbPB->setEnabled(true);
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
        ui->uploadLdbPB->setEnabled(true);
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
        err_str = "编号为空，将自动设为当前日期+时间。请确认是否继续！";
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
        if(curr_no.isEmpty())
        {
            /*编号为空，自动设置为当前日期和时间*/
            QString synthe_no = QString(m_def_obj_id_prefix)
                                + diy_curr_date_time_str_ms(false);
            ui->numberTextEdit->setText(synthe_no);
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
    if(ui->devIDTextEdit->text().isEmpty()
            || ui->exptIDTextEdit->text().isEmpty())
    {
        /*check dev_id and experiment id. They are as primary key in database,
         * so can't be empty. If empty, set to default value.
        */
        QString err_str = "实验ID和设备ID不能为空。如果不填写，将使用自动生成的ID。是否继续？";
        QMessageBox::StandardButton sel;
        sel = QMessageBox::question(nullptr, "？？？", err_str);
        if(QMessageBox::No == sel)
        {
            return false;
        }
        if(ui->devIDTextEdit->text().isEmpty())
        {
            ui->devIDTextEdit->setText(QString(m_def_dev_id_prefix)
                                       + m_work_dev_info->addr);
        }
        if(ui->exptIDTextEdit->text().isEmpty())
        {
            ui->exptIDTextEdit->setText(QString(m_def_expt_id_prefix)
                                        + diy_curr_date_time_str_ms(false));
        }
    }

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
        disable_data_collection_btns();

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
                                       m_curr_light_no->idx, m_work_dev_info->dev_type);
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
                                 ORID_DATA_BYTES_NUM), "").toUtf8();
    QByteArray lambda_value
            = QByteHexString(QByteArray(&(value.constData()[ORID_LAMBDA_POS_START]),
                                 ORID_LAMBDA_BYTES_NUM), "").toUtf8();

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

        ld.lambda = m_curr_light_no->lambda;
        if(0 == ld.lambda)
        {
            ld.lambda = lambda_value.toUInt(nullptr, 16);
        }
        ld.lambda = lambda_correction(ld.lambda);
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
                                                      m_curr_light_no->idx,
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
                    arg(m_curr_light_no->lambda).\
                    arg(m_curr_light_no->idx);
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
    ui->uploadLdbPB->setEnabled(true);
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
    if(directory.isEmpty())
    {
        QString hint_str;
        hint_str = QString("未选择合适的文件夹，数据文件将存储在下面的位置：")
                + m_data_pth_str;
    }
    else
    {
        directory.replace("\\","/"); //双斜杠转换单斜杠
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

void Chat::draw_data_from_file(QFile &txt_f, lambda_data_map_t &l_d, bool visual_lambda_map)
{
     if(!txt_f.open(QIODevice::ReadOnly | QIODevice::Text))
     {
         DIY_LOG(LOG_LEVEL::LOG_WARN, "Read file error: %ls", txt_f.fileName().utf16());
         return;
     }

     QByteArray b_arr;
     quint32 lambda;
     quint64 data;
     QString line;
     QTextStream in(&txt_f);
     while (!in.atEnd()) {
         line = in.readLine();
         line.remove(" ");
         if(line.isEmpty())
         {
             continue;
         }
         b_arr = line.toUtf8();//hex_str_to_byte_array(line);
         ble_comm_get_lambda_data_from_pkt(b_arr, lambda, data);
         if(visual_lambda_map)
         {
             lambda = lambda_correction(lambda,
                                        &m_sw_settings.oth_settings.visual_lambda_corr);
         }
         l_d.insert(lambda, data);
     }
     txt_f.close();
}

void Chat::add_lambda_data_map(lambda_data_map_t &l_d, x_axis_values_t &x_l_d)
{
    qsizetype list_len;

    if(l_d.isEmpty())
    {
        return;
    }

    if(x_l_d.isEmpty())
    {
        list_len = 0;
    }
    else
    {
        list_len = x_l_d.begin()->length();
    }

    foreach(quint32 lambda, x_l_d.keys())
    {
        if(l_d.contains(lambda))
        {
            x_l_d[lambda].append(l_d.value(lambda));
        }
        else
        {
            x_l_d[lambda].append(m_invalid_ldata);
        }
    }

    foreach(quint32 lambda, l_d.keys())
    {
        if(!x_l_d.contains(lambda))
        {
            QList<quint64> dlist(list_len, m_invalid_ldata);
            dlist.append(l_d.value(lambda));
            x_l_d.insert(lambda, dlist);
        }
    }
}

void Chat::display_lambda_data_lines(x_axis_values_t &x_l_d, QString save_pth)
{
    ChartDisplayDlg display_dlg(save_pth);
    QString err;
    if(x_l_d.isEmpty())
    {
        err = "没有可供显示的数据！";
        DIY_LOG(LOG_LEVEL::LOG_ERROR, "%ls", err.utf16());
        QMessageBox::critical(nullptr, "!!!", err);
        return;
    }

    qsizetype series_count = x_l_d.begin().value().length();
    //d_max and d_min record the max and min value of all data, exclude m_invalid_ldata;
    quint64 d_min = m_invalid_ldata, d_max = 0, c_m;
    //the two lists record the max and min value at each position(lambda), exclude m_invalid_ldata;
    QList<quint64> point_max_l, point_min_l;
    x_axis_values_t::iterator x_it = x_l_d.begin();
    while(x_it != x_l_d.end())
    {
        if(x_it.value().length() != series_count)
        {
            err = "内部数据系列构造错误：各波段列表长度不一致！";
            DIY_LOG(LOG_LEVEL::LOG_ERROR, "%ls: series_count is %d, one count is %d",
                    err.utf16(), series_count, x_it.value().length());
            QMessageBox::critical(nullptr, "!!!", err);
            return;
        }
        /*find min value at this position(lambda)*/
        c_m = *std::min_element(x_it.value().begin(), x_it.value().end());
        point_min_l.append(c_m);
        if(c_m < d_min) d_min = c_m;
        /*find max value at this position(lambda)*/
        c_m = 0;
        for(qsizetype i = 0; i < series_count; i++)
        {
            quint64 d;
            d = x_it.value().value(i);
            if((d != m_invalid_ldata) && (d > c_m))
            {
                c_m = d;
            }
        }
        point_max_l.append(c_m);
        if(c_m > d_max) d_max = c_m;

        ++x_it;
    }

    QList<QLineSeries*> line_series_list;
    for(int i = 0; i < series_count; i++)
    {
        QLineSeries * l_s = new QLineSeries();
        if(nullptr == l_s)
        {
            qDeleteAll(line_series_list);
            line_series_list.clear();

            err = "创建LineSeries失败！";
            DIY_LOG(LOG_LEVEL::LOG_ERROR, "%ls", err.utf16());
            QMessageBox::critical(nullptr, "!!!", err);
            return;
        }
        line_series_list.append(l_s);
        l_s->setPointsVisible();
        l_s->setPointLabelsFormat("@yPoint");
        l_s->setPointLabelsVisible(false);
    }
    if(1 == series_count)
    {
        line_series_list[0]->setPointLabelsVisible(true);
    }

    QStringList x_axis_tick_list;
    x_it = x_l_d.begin();
    int x_val = 0;
    quint64 cur_d;
    bool min_displayed, max_displayed;
    while(x_it != x_l_d.end())
    {
        /*for x axis generation*/
        x_axis_tick_list.append(QString::number(x_it.key()));

        /*generate line series*/
        min_displayed = max_displayed = false;
        for(int i = 0; i < series_count; i++)
        {
            cur_d = x_it.value().value(i);

            if(m_invalid_ldata == cur_d)
            {
                line_series_list[i]->append(QPointF(x_val, 0));
                line_series_list[i]->setPointConfiguration(x_val,
                                                   QXYSeries::PointConfiguration::Visibility,
                                                   false);
                line_series_list[i]->setPointConfiguration(x_val,
                                                   QXYSeries::PointConfiguration::LabelVisibility,
                                                   false);
            }
            else
            {
                line_series_list[i]->append(QPointF(x_val, cur_d));
                if(series_count > 1)
                {
                    if(!max_displayed && (cur_d == point_max_l[x_val]))
                    {
                        line_series_list[i]->setPointConfiguration(x_val,
                                                           QXYSeries::PointConfiguration::LabelVisibility,
                                                           true);
                        max_displayed = true;
                    }
                    if(!min_displayed && (cur_d == point_min_l[x_val]))
                    {
                        line_series_list[i]->setPointConfiguration(x_val,
                                                           QXYSeries::PointConfiguration::LabelVisibility,
                                                           true);
                        min_displayed = true;
                    }
                }
            }
        }

        ++x_val;
        ++x_it;
    }
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(x_axis_tick_list);
    axisX->setGridLineVisible(false);
    axisX->setTitleText("波长（nm）");
    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(d_min, d_max * 1.1);
    axisY->setGridLineVisible(false);

    QChart * chart = new QChart();
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    foreach(QLineSeries * l_s, line_series_list)
    {
        chart->addSeries(l_s);
        l_s->attachAxis(axisX);
        l_s->attachAxis(axisY);
    }
    chart->legend()->setVisible(false);

    QChartView * chartView = new QChartView(chart, &display_dlg);
    chartView->setRenderHint(QPainter::Antialiasing);

    display_dlg.arrange_gv_display(chartView);

    display_dlg.exec();

    /* Do not need to free resource explicitly. Since they all are child/grand-child
     * of display_dlg, the latter will release these resource when destructed.
    */
}

void Chat::on_fileVisualButton_clicked()
{
    QString file_pos, path_str;
    QStringList file_list;
    if(ui->currFileradioButton->isChecked())
    {
        path_str = m_data_pth_str + "/" + QString(m_txt_dir_rel_name);
        file_pos = path_str + "/" + m_curr_file_bn_str + m_txt_ext;

        QFile f(file_pos);
        if(!f.exists())
        {
            QMessageBox::warning(nullptr, "!!!", "尚未采集数据！");
            return;
        }
        file_list.append(file_pos);
    }
    else if(ui->otherFileradioButton->isChecked())
    {
        QString fpn = QFileDialog::getOpenFileName(this, tr("请选择文件"),
                                                 QDir::currentPath(),
                                                 tr(QString("Text Files(*%1)").arg(m_txt_ext).toUtf8().constData()));
        if(fpn.isEmpty())
        {
            return;
        }
        else
        {
            path_str = QFileInfo(fpn).absolutePath();
            file_pos = fpn;
            ui->currFileNameLabel->setText(QFileInfo(fpn).fileName());
            file_list.append(file_pos);
        }
    }
    else if(ui->currFolderradioButton->isChecked())
    {
        file_pos = m_data_pth_str + "/" + m_txt_dir_rel_name;
        path_str = file_pos;

        /*traverse the dir and get txt file list.*/
        get_dir_content_fpn_list(file_pos, file_list,
                                 QDir::Filter::Files, QDir::SortFlag::Name, m_txt_ext);
        if(file_pos.isEmpty())
        {
            QMessageBox::warning(nullptr, "", "没有找到可供显示的文件");
            return;
        }
    }
    else if(ui->otherFolderVisualButton->isChecked())
    {
        file_pos = QFileDialog::getExistingDirectory(this,tr("选择文件夹"),QDir::currentPath());
        if(file_pos.isEmpty())
        {
            return;
        }
        get_dir_content_fpn_list(file_pos, file_list,
                                 QDir::Filter::Files, QDir::SortFlag::Name, m_txt_ext);
        if(file_pos.isEmpty())
        {
            QMessageBox::warning(nullptr, "", "没有找到可供显示的文件");
            return;
        }
        path_str = file_pos;
    }
    else
    {
        QMessageBox::warning(nullptr, "!!!", "请选择显示文件还是文件夹内容");
        return;
    }

    QFile qf;
    lambda_data_map_t l_d;
    foreach(const QString &s, file_list)
    {
        qf.setFileName(s);
        l_d.clear();
        draw_data_from_file(qf, l_d);
        add_lambda_data_map(l_d, m_x_axis_values);
    }
    display_lambda_data_lines(m_x_axis_values, path_str);

    x_axis_values_t::iterator x_axis_it = m_x_axis_values.begin();
    while(m_x_axis_values.end() != x_axis_it)
    {
        x_axis_it.value().clear();
        ++x_axis_it;
    }
    m_x_axis_values.clear();

    return;

    /* The following code uses external python program to display. Now
     * we use Qt internal function, so these are not used.
    */
    /*
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
    */
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

void Chat::push_pop_data_collection_btns(chat_push_pop_btn_t p_p)
{
    static bool send_btn = ui->sendButton->isEnabled();
    static bool calib_btn = ui->calibrationButton->isEnabled();
    static bool manual_btn = ui->manualCmdBtn->isEnabled();
    static bool upload_btn = ui->uploadLdbPB->isEnabled();

    if(PUSH_BTNS == p_p)
    {
        send_btn = ui->sendButton->isEnabled();
        calib_btn = ui->calibrationButton->isEnabled();
        manual_btn = ui->manualCmdBtn->isEnabled();
        upload_btn = ui->uploadLdbPB->isEnabled();
    }
    else
    {
        ui->sendButton->setEnabled(send_btn);
        ui->calibrationButton->setEnabled(calib_btn);
        ui->manualCmdBtn->setEnabled(manual_btn);
        ui->uploadLdbPB->setEnabled(upload_btn);
    }
}

void Chat::disable_data_collection_btns()
{
    ui->sendButton->setEnabled(false);
    ui->calibrationButton->setEnabled(false);
    ui->manualCmdBtn->setEnabled(false);
    ui->uploadLdbPB->setEnabled(false);
}

void Chat::show_rdb_wait_box(bool show, QString title, QString box_str,
                           QMessageBox::StandardButtons buttons)
{
    if(!show)
    {
        m_remote_db_wait_box.hide();
    }
    else
    {
        m_remote_db_wait_box.setWindowTitle(title);
        m_remote_db_wait_box.setText(box_str);
        m_remote_db_wait_box.setStandardButtons(buttons);
        m_remote_db_wait_box.setWindowFlags(Qt::FramelessWindowHint | Qt::CustomizeWindowHint);
        m_remote_db_wait_box.setWindowModality(Qt::NonModal);
        m_remote_db_wait_box.show();
    }
}

void Chat::on_quitButton_clicked()
{
    if(m_skin_db)
    {
        if(m_writing_rdb)
        {
            show_rdb_wait_box(true, "", QString("上传远程数据库尚未完成，请稍等..."),
                              QMessageBox::StandardButton::Ok);
            return;
        }
        else
        {
            m_skin_db->close_dbs(SkinDatabase::DB_ALL);
            show_rdb_wait_box(false);
            if(!m_user_close_hint_msg.isEmpty())
            {
                QMessageBox::warning(nullptr, "!!!",  m_user_close_hint_msg);
            }
        }
    }
    this->accept();
}

void Chat::closeEvent(QCloseEvent *event)
{
    if(m_skin_db)
    {
        if(m_writing_rdb)
        {
            show_rdb_wait_box(true, "", QString("上传远程数据库尚未完成，请稍等..."),
                              QMessageBox::StandardButton::Ok);
            event->ignore();
            return;
        }
        else
        {
            m_skin_db->close_dbs(SkinDatabase::DB_ALL);
            show_rdb_wait_box(false);
            if(!m_user_close_hint_msg.isEmpty())
            {
                QMessageBox::warning(nullptr, "!!!",  m_user_close_hint_msg);
            }
        }
    }
    event->accept();
}

void Chat::rdb_write_start_handler()
{
    m_writing_rdb = true;
    ui->stateIndTextEdit->setText("同步上传远程数据库......");
}

void Chat::rdb_write_done_hanlder(SkinDatabase::db_ind_t write_ind, bool /*ret*/)
{
    QString result_str, part_ind_str = QString("部分");
    static bool no_db_save_rec = false, part_up_rec = false;

    m_writing_rdb = false;
    show_rdb_wait_box(false);
    if(m_skin_db)
    {
        m_skin_db->close_dbs(SkinDatabase::DB_ALL);
    }
    if(SkinDatabase::DB_NONE == write_ind)
    {
        result_str = QString("数据未能上传到远程数据库，也未能在本地%1文件夹中保存！\n"
                             "您可以从本地的csv文件和%2文件夹中查找数据。建议后续将这些数据手动上传到远程数据库作长久保存！")
                .arg(m_safe_ldb_for_rdb_rel_name, m_local_db_dir_rel_name);
        if(!no_db_save_rec)
        {
            if(m_user_close_hint_msg.isEmpty())
            {
                m_user_close_hint_msg = result_str;
            }
            else
            {
                m_user_close_hint_msg = result_str + "\n" + m_user_close_hint_msg;
            }
            if(part_up_rec)
            {
                m_user_close_hint_msg = part_ind_str + m_user_close_hint_msg;
            }
        }
        no_db_save_rec = true;
    }
    else if(write_ind & (SkinDatabase::DB_SAFE_LDB))
    {
        result_str = QString("数据未能成功上传到远程数据库。");
        if(write_ind & (SkinDatabase::DB_REMOTE))
        {
            result_str = part_ind_str + result_str;
        }

        result_str += QString("它们已经在本地%1文件中保存。\n"
                             "后续您可以点击\"上传safe ldb\"按钮选择该文件，将其中的数据上传到远程数据库作长久保存。")
                .arg(m_safe_ldb_for_rdb_rel_name);

        if(!part_up_rec)
        {
            if(!m_user_close_hint_msg.isEmpty())
            {
                m_user_close_hint_msg += "\n";
            }
            m_user_close_hint_msg += result_str;
            if(no_db_save_rec)
            {
                m_user_close_hint_msg = part_ind_str + m_user_close_hint_msg;
            }
        }
        part_up_rec = true;
    }
    else
    {
        result_str = QString("数据已同步上传到远程数据库保存");
    }

    ui->stateIndTextEdit->setText(result_str);

}

void Chat::upload_safe_ldb_end_handler(QList<SkinDatabase::tbl_rec_op_result_t> op_result,
                                       bool result_ret)
{
    show_rdb_wait_box(false);

    m_upload_safe_ldb_now = false;
    QString result_str = QString("上传结果：\n");
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
        result_str += "所有记录均已成功上传到服务器！\n";
        if(result_ret)
        {
            result_str += "safe ldb文件已删除。\n";
        }
        else
        {
            result_str += "但删除safe ldb文件失败。请手动删除。";
        }
    }
    else
    {
        result_str += "未能成功上传全部记录。请查看log文件。";
    }
    QMessageBox::information(nullptr, "Result", result_str);

    push_pop_data_collection_btns(Chat::POP_BTNS);
}

void Chat::select_safe_ldb_for_upload()
{
    QString fpn = QFileDialog::getOpenFileName(this, tr("请选择文件"),
                                             QDir::currentPath(), tr("(*.sqlite)"));
    if(fpn.isEmpty())
    {
        push_pop_data_collection_btns(Chat::POP_BTNS);
        return;
    }
    else
    {
        ui->safeldbLabel->setText(QFileInfo(fpn).fileName());

        m_safe_ldb_for_upload_fpn = fpn;
        m_skin_db->upload_safe_ldb(m_safe_ldb_for_upload_fpn);

        show_rdb_wait_box(true, "", "Uploading...");
    }
}

void Chat::rdb_state_upd_handler(SkinDatabase::rdb_state_t rdb_st)
{
    show_rdb_wait_box(false);
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
            push_pop_data_collection_btns(Chat::POP_BTNS);
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
    push_pop_data_collection_btns(Chat::PUSH_BTNS);
    disable_data_collection_btns();
    m_upload_safe_ldb_now = true;
    if(m_skin_db->remote_db_st() == SkinDatabase::RDB_ST_OK)
    {
        select_safe_ldb_for_upload();
    }
    else
    {
        m_skin_db->prepare_remote_db();

        show_rdb_wait_box(true, "", "Remote db preparing...");
    }
}

quint32 Chat::lambda_correction(quint32 lambda, QMap<quint32, quint32>* corr_map)
{
    if(nullptr == corr_map)
    {
        if(m_work_dev_info && !(m_work_dev_info->dev_lambda_corr.isEmpty()))
        {
            corr_map = &(m_work_dev_info->dev_lambda_corr);
        }
        else if(!(m_sw_settings.oth_settings.global_lambda_corr.isEmpty()))
        {
            corr_map = &(m_sw_settings.oth_settings.global_lambda_corr);
        }
        else
        {
            return lambda;
        }
    }
    if(corr_map->contains(lambda))
    {
        return (*corr_map)[lambda];
    }
    else
    {
        return lambda;
    }
}
