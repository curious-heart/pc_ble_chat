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

#include "ui_chat.h"

#include <QtWidgets/qdialog.h>
#include "remoteselector.h"
#include <QtBluetooth/qbluetoothhostinfo.h>
#include "characteristicinfo.h"
#include "serviceinfo.h"
#include "diy_common_tool.h"
#include <QFile>
#include <QDir>
#include <QTimer>
#include <QFileDialog>
#include <QProcess>

QT_USE_NAMESPACE

class ChatServer;
class ChatClient;

//! [declaration]
class Chat : public QDialog
{
    Q_OBJECT

public:
    explicit Chat(QWidget *parent = nullptr);
    ~Chat();

signals:
    void sendMessage(const QString &message);
    void write_data_done_sig();

private slots:
    void connectClicked();
    void sendClicked();

    void showMessage(const QString &sender, const QString &message);

    void clientConnected(const QString &name);
    void clientDisconnected(const QString &name);
    void clientDisconnected();
    void connected(const QString &name);
    void reactOnSocketError(const QString &error);

    void newAdapterSelected();

    void BleServiceCharacteristicWrite(const QLowEnergyCharacteristic &c,
                                       const QByteArray &value);
    void BleServiceCharacteristicRead(const QLowEnergyCharacteristic &c,
                                       const QByteArray &value);
    void BleServiceCharacteristicChanged(const QLowEnergyCharacteristic &c,
                                         const QByteArray &value);
    void write_data_done_handle();

    void on_chat_textChanged();

    void on_disconnButton_clicked();

    void on_clearButton_clicked();

    void on_calibrationButton_clicked();

    void on_choosePathButton_clicked();

    void on_allDevcheckBox_stateChanged(int arg1);

    void on_onlyValidDatacheckBox_stateChanged(int arg1);

    void on_fileVisualButton_clicked();

    void on_currFileradioButton_clicked();

    void on_folderradioButton_clicked();

private:
    int adapterFromUserSelection() const;
    int currentAdapterIndex = 0;
    Ui_Chat *ui;

    ChatServer *server = nullptr;
    QList<ChatClient *> clients;
    QList<QBluetoothHostInfo> localAdapters;

    QString localName;
    QBluetoothAddress m_adapter;

    RemoteSelector* m_remoteSelector = nullptr;
    QList<QByteArray> m_write_data_list;
    static const int m_light_num = 19;
    static const int m_write_data_len = 20;
    static const unsigned char m_light_idx_pos = 2;
    /*data start with "0x5A 0x11" are considered valid.*/
    const QByteArray m_valid_data_flag = "\x5A\x11";
    ServiceInfo * m_service = nullptr;
    CharacteristicInfo* m_rx_char = nullptr, *m_tx_char = nullptr;

    bool m_file_write_ready = false;
    bool m_dir_ready = false;
    const char* m_data_dir_rel_name = "0000-light_data_dir";
    const char* m_redundant_dir_rel_name = "ori_data";
    const char* m_file_name_apx = "_all";
    QString m_data_file_pth_str, m_redundant_file_path_str;
    QFile m_file, m_valid_data_file;
    bool m_single_light_write = false;
    bool m_calibrating = false;
    const char* m_calibration_file_name_apx = "校准";
    const char* m_data_file_type_str = ".txt";
    bool m_all_dev_scan = false;
    bool m_only_rec_valid_data = true;
    QString m_current_valid_data_file_name;
    QString m_data_no;

    QTimer m_write_done_timer;

    const char* m_visual_exe_fpn = "../vway_data_visual/dist/vway_data_visual/vway_data_visual.exe";
    void init_write_data();
    void read_notify();
    void write_data_done_notify();
    void restart_work();
    void send_data_to_device();

public:
};
//! [declaration]
