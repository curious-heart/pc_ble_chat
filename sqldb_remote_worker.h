#ifndef SQLDB_REMOTE_WORKER_H
#define SQLDB_REMOTE_WORKER_H

#include <QObject>
#include <QSqlDatabase>

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
public slots:
    bool prepare_rdb(setting_db_info_t db_info);
    bool write_rdb(SkinDatabase::db_info_intf_t intf);
    bool close_rdb();

signals:
    void remote_db_prepared();
    void remote_db_write_done();
    void remote_db_write_error();
    void remote_db_closed();
};

#endif // SQLDB_REMOTE_WORKER_H
