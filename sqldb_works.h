#ifndef SQLDB_WORKS_H
#define SQLDB_WORKS_H

#include <QString>
#include <QStringList>
#include <QSqlDatabase>
#include <QMessageBox>
#include <QList>
#include <QMap>

#include "diy_common_tool.h"
/*
 * local db: use sqlite to store data into a local db file.
 * remote db: use mysql server.
*/
class SkinDatabase
{
private:
    setting_db_info_t *m_remote_db_info = nullptr;

    QString m_local_db_pth_str;
    QString m_local_db_name_str;
    bool m_local_db_ready = false;

    QSqlDatabase m_local_db, m_remote_db;

public:
    typedef struct
    {
        quint32 lambda;
        quint64 data;
    }db_lambda_data_s_t;
    typedef struct
    {
        QString expt_id;
        QString expt_desc;
        QString expt_date;
        QString expt_time;
        QString obj_id;
        QString skin_type;
        QString obj_desc;
        QString dev_id;
        QString dev_addr;
        QString dev_desc;
        QString pos;
        QList<db_lambda_data_s_t> lambda_data;
        QString rec_date;
        QString rec_time;
    }db_info_intf_t;

private:
    db_info_intf_t m_intf;
    bool prepare_local_db();
    bool create_tbls_and_views();

public:
    SkinDatabase();
    ~SkinDatabase();

    void set_remote_db_info(setting_db_info_t * db_info);
    void set_local_db_pth_str(QString str);
    void store_these_info(db_info_intf_t &info);
};

#endif // SQLDB_WORKS_H
