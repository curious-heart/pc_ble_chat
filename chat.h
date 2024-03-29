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
#include <QThread>
#include "remoteselector.h"
#include <QtBluetooth/qbluetoothhostinfo.h>
#include "characteristicinfo.h"
#include "serviceinfo.h"
#include <QFile>
#include <QDir>
#include <QTimer>
#include <QFileDialog>
#include <QProcess>
#include <QDomDocument>
#include <QList>
#include <QCloseEvent>
#include <QMessageBox>

#include "sw_setting_parse.h"
#include "sqldb_works.h"

QT_USE_NAMESPACE

//! [declaration]
class Chat : public QDialog
{
    Q_OBJECT

public:
    explicit Chat(QWidget *parent = nullptr);
    ~Chat();

signals:
    void sendMessage(const QString &message);
    void write_data_done_sig(bool done);
    void turn_on_next_light_sig(light_list_t::Iterator no);

private slots:
    void connectClicked();
    void sendClicked();

    void showMessage(const QString &sender, const QString &message);

    void clientConnected(const QString &name);
    void connected(const QString &name);
    void reactOnSocketError(const QString &error);

    void newAdapterSelected();

    void BleServiceCharacteristicWritten(const QLowEnergyCharacteristic &c,
                                       const QByteArray &value);
    void BleServiceCharacteristicRead(const QLowEnergyCharacteristic &c,
                                       const QByteArray &value);
    void BleServiceCharacteristicChanged(const QLowEnergyCharacteristic &c,
                                         const QByteArray &value);
    void BleServiceError(QLowEnergyService::ServiceError newError);

    void write_data_done_handle(bool done);
    void turn_on_next_light(light_list_t::Iterator no);

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

    void on_manualCmdBtn_clicked();

    void on_sendText_textEdited(const QString &arg1);

    void on_quitButton_clicked();

    void on_uploadLdbPB_clicked();

    void rdb_state_upd_handler(SkinDatabase::rdb_state_t rdb_st);
    void upload_safe_ldb_end_handler(QList<SkinDatabase::tbl_rec_op_result_t> op_result,
                                     bool result_ret);
    void rdb_write_start_handler();
    void rdb_write_done_hanlder(SkinDatabase::db_ind_t write_ind, bool ret);

private:
    int adapterFromUserSelection() const;
    int currentAdapterIndex = 0;
    Ui_Chat *ui;
    bool m_first_check = true;

    QList<QBluetoothHostInfo> localAdapters;

    QString localName;
    QBluetoothAddress m_adapter;

    RemoteSelector* m_remoteSelector = nullptr;
    //QList<QByteArray> m_write_data_list;
    static const int m_write_data_len = 20;
    static const unsigned char m_light_idx_pos = 2;
    ServiceInfo * m_service = nullptr;
    CharacteristicInfo* m_rx_char = nullptr, *m_tx_char = nullptr;
    setting_ble_dev_info_t * m_work_dev_info = nullptr;

    bool m_file_write_ready = false;
    bool m_dir_ready = false;
    const char* m_def_expt_id_prefix = "expt-";
    const char* m_def_obj_id_prefix = "obj-";
    const char* m_def_dev_id_prefix = "dev-";
    const char* m_data_dir_rel_name = "0000-light_data_dir";
    const char* m_all_rec_dir_rel_name = "ori_data";
    const char* m_txt_dir_rel_name = "txt";
    const char* m_csv_dir_rel_name = "csv";
    const char* m_local_db_dir_rel_name = "local_db";
    const char* m_all_rec_file_name_apx = "_all";
    QString m_data_pth_str; /*the "root" path to store received data.*/
    QString m_all_rec_pth_str; /*all received data are stored here.*/
    QString m_txt_pth_str; /*valid data file are stored here in txt format.*/
    QString m_csv_pth_str; /*valid data file are stored here in csv format.*/
    QString m_local_db_pth_str;
    QFile m_all_rec_file, m_txt_file;
    bool m_single_light_write = false;
    bool m_calibrating = false;
    const char* m_calibration_no = "0000", *m_calibration_pos = "cal";
    const char* m_calibration_file_name_apx = "_校准";
    const char* m_txt_ext = ".txt";
    bool m_all_dev_scan = false;
    bool m_only_rec_valid_data = true;
    QString m_curr_file_bn_str = "";
    light_list_t::Iterator m_curr_light_no; /* used to traverse light_list.*/
    SkinDatabase *m_skin_db = nullptr;
    SkinDatabase::db_info_intf_t m_db_data;
    /*records that to be stored in remote db but failed are stored here,
     * so that later user can stored them into remote db manually.
     */
    const char* m_safe_ldb_for_rdb_dir_rel_name ="safe_ldb_for_rdb";
    const char* m_safe_ldb_for_rdb_rel_name = "safe_ldb_for_rdb.sqlite";

    QTimer m_write_wait_resp_timer;
    QTimer m_write_done_timer;

    /*
     * ./m_visual_exe_fpn
     * or
     * ../m_visual_exe_fpn
    */
    const char* m_visual_exe_fpn = "vway_data_visual/dist/vway_data_visual/vway_data_visual.exe";
    //void init_write_data(); //no use now. we generate data packet dynamicly.
    void write_data_done_notify();
    void write_wait_resp_timeout();
    void restart_work();
    void start_send_data_to_device(bool single_cmd);
    bool check_volu_info();
    bool check_and_mkpth();
    bool prepare_qfile_for_start();
    void set_manual_cmd_btn();

    sw_settings_t m_sw_settings;

    QThread m_log_thread;

    bool m_writing_rdb = false;
    QString m_user_close_hint_msg = "";
    QString m_safe_ldb_for_upload_fpn = "";
    bool m_upload_safe_ldb_now = false;
    void select_safe_ldb_for_upload();
    QMessageBox m_remote_db_wait_box;
    int show_rdb_wait_box(bool show, QString title = "", QString box_str = "",
                         QMessageBox::StandardButtons buttons = QMessageBox::NoButton);
    typedef enum
    {
        PUSH_BTNS,
        POP_BTNS
    }chat_push_pop_btn_t;
    void push_pop_data_collection_btns(chat_push_pop_btn_t p_p);
    void enable_data_collection_btns(bool en = true);
    void toggle_conn_btn(bool);

    const quint64 m_invalid_ldata = (quint64)(-1);
    typedef QMap<quint32, quint64> lambda_data_map_t;
    typedef QMap<quint32, QList<quint64>> x_axis_values_t;
    x_axis_values_t m_x_axis_values;
    void draw_data_from_file(QFile &txt_f, lambda_data_map_t &l_d, bool visual_lambda_map = true);
    void add_lambda_data_map(lambda_data_map_t &l_d, x_axis_values_t &x_l_d);
    void display_lambda_data_lines(x_axis_values_t &x_l_d, QString save_pth = "");
    quint32 lambda_correction(quint32 lambda, QMap<quint32, quint32>* corr_map = nullptr);

protected:
    void closeEvent(QCloseEvent * event);
};
//! [declaration]
