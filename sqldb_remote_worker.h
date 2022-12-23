#ifndef SQLDB_REMOTE_WORKER_H
#define SQLDB_REMOTE_WORKER_H

#include "diy_common_tool.h"
#include "sqldb_works.h"

class SqlDbRemoteWorker: public QObject
{
    Q_OBJECT
public:
    SqlDbRemoteWorker();
    ~SqlDbRemoteWorker();


private:
    bool m_remote_db_ready = false;
    QString m_safe_ldb_dir_str = "", m_safe_ldb_file_str = "";
    bool m_safe_ldb_ready = false;

public slots:
    bool prepare_rdb(setting_rdb_info_t db_info, QString safe_ldb_dir_str,
                                        QString safe_ldb_file_str);
    bool write_rdb(SkinDatabase::db_info_intf_t intf, setting_rdb_info_t rdb_info,
                   QString safe_ldb_dir_str, QString safe_ldb_file_str);
    bool close_rdb(SkinDatabase::db_ind_t);
    bool upload_safe_ldb_to_rdb(QString safe_ldb_fpn);

signals:
    void remote_db_prepared_sig(bool ok);
    void remote_db_write_done_sig();
    void remote_db_write_error_sig();
    void remote_db_closed_sig();
    void upload_safe_ldb_done_sig();
};

#endif // SQLDB_REMOTE_WORKER_H
