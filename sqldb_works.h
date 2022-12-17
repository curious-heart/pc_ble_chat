#ifndef SQLDB_WORKS_H
#define SQLDB_WORKS_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QSqlDatabase>
#include <QMessageBox>
#include <QList>
#include <QMap>
#include <QFile>
#include <QThread>

#include "diy_common_tool.h"
/*
 * local db: use sqlite to store data into a local db file.
 * remote db: use mysql server.
*/
class SkinDatabase:public QObject
{
    Q_OBJECT

private:
    setting_db_info_t *m_remote_db_info = nullptr;

    QString m_local_db_pth_str, m_local_csv_pth_str;
    QString m_local_db_name_str, m_local_csv_name_str;
    bool m_local_db_ready = false, m_local_csv_ready = false;

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
        bool expt_changed;
        bool obj_changed;
        bool dev_changed;
    }db_info_intf_t;

    typedef enum
    {
        LOCAL,
        REMOTE,
    }db_pos_t;
    typedef enum
    {
        DB_NONE,
        DB_SQLITE,
        DB_MYSQL,
    }db_type_t;
private:
    db_info_intf_t m_intf;
    bool prepare_local_db();
    bool write_local_db(QSqlDatabase &qdb, db_info_intf_t &intf, db_type_t db_type);

    QFile m_local_csv_f;
    bool prepare_local_csv();
    bool write_local_csv(db_info_intf_t &intf);

    static bool write_db(QSqlDatabase &qdb, db_info_intf_t &intf,
                         db_pos_t db_pos, db_type_t db_type);
public:
    SkinDatabase(setting_db_info_t * rdb_info = nullptr);
    ~SkinDatabase();

    void set_remote_db_info(setting_db_info_t * db_info);
    void set_local_store_pth_str(QString db, QString csv);
    bool store_these_info(db_info_intf_t &info);
    static bool create_tbls_and_views(QSqlDatabase &qdb,
                                      db_pos_t db_pos, db_type_t db_type);
    static bool write_remote_db(QSqlDatabase &qdb, db_info_intf_t &intf,
                                db_type_t db_type);
signals:
    bool prepare_rdb_sig(setting_db_info_t db_info);
    bool write_rdb_sig(SkinDatabase::db_info_intf_t intf);
    bool close_rdb_sig();

private:
    QThread m_rdb_thread;
};

/*sqlerr must be of type QSqlError*/
#define SQL_LAST_ERR_STR(sqlerr)\
            (QString("err_txt:") + (sqlerr).text() + "\n"\
             + "native_code:" + (sqlerr).nativeErrorCode() + "; "\
             + "err type:" + QString("%1").arg((int)((sqlerr).type())))

void remove_qt_sqldb_conn(QString conn_name);
#endif // SQLDB_WORKS_H
