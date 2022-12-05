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
    const QString m_db_name_str{"detector"};
    const QString m_tbl_expts_str{"experiments"};
    const QString m_col_expt_id_str{"expt_id"};
    const QString m_col_desc_str{"desc"};
    const QString m_col_date_str{"date"};
    const QString m_tbl_objects_str{"objects"};
    const QString m_col_obj_id_str{"obj_id"};
    const QString m_col_skin_type_str{"skin_type"};
    const QString m_tbl_devices_str{"devices"};
    const QString m_col_dev_id_str{"dev_id"};
    const QString m_col_dev_addr_str{"dev_addr"};
    const QString m_tbl_datum_str{"datum"};
    const QString m_col_pos_str{"pos"};
    const QString m_col_lambda_str{"lambda"};
    const QString m_col_data_str{"data"};
    const QString m_col_time_str{"time"};
    const QString m_col_rec_id_str{"rec_id"};
    const QString m_view_datum_str{"datum_view"};
    inline QString expt_tbl_col_def()
    {
        return m_col_expt_id_str + " TEXT," + m_col_desc_str + " TEXT,"
               + m_col_date_str + " TEXT," + m_col_time_str + " TEXT,"
                + "PRIMARY KEY(" + m_col_expt_id_str + ")";
    }
    inline QString objects_tbl_col_def()
    {
        return m_col_obj_id_str + " TEXT," + m_col_skin_type_str + " TEXT,"
                + m_col_desc_str + " TEXT,"
                + "PRIMARY KEY(" + m_col_obj_id_str + ")";
    }
    inline QString devices_tbl_col_def()
    {
        return m_col_dev_id_str + " TEXT," + m_col_dev_addr_str + " TEXT,"
                + m_col_desc_str + " TEXT,"
                + "PRIMARY KEY(" + m_col_dev_id_str + ")";
    }
    inline QString datum_tbl_col_def()
    {
        return m_col_obj_id_str + " TEXT," + m_col_pos_str + " TEXT,"
                + m_col_lambda_str + " INT," + m_col_data_str + " UNSIGNED BIG INT,"
                + m_col_date_str + " TEXT," + m_col_time_str + " TEXT,"
                + m_col_dev_id_str + " TEXT," + m_col_expt_id_str + " TEXT,"
                + m_col_rec_id_str + " TEXT,"
                + "PRIMARY KEY(" + m_col_rec_id_str + ")";
    }
    inline QString datum_view_col_def()
    {
        return m_col_dev_id_str + " TEXT," + m_col_obj_id_str + " TEXT,"
                + m_col_skin_type_str + " TEXT," + m_col_pos_str + " TEXT,"
                + m_col_lambda_str + " INT," + m_col_data_str + " UNSIGNED BIG INT,"
                + m_col_date_str + " TEXT," + m_col_time_str + " TEXT,"
                + m_col_expt_id_str + " TEXT,";
    }

    typedef QString (SkinDatabase::*col_def_func_t)();
    QMap<QString, col_def_func_t> m_tbl_create_fac;
    QMap<QString, col_def_func_t> m_view_create_fac;

    setting_db_info_t *m_remote_db_info = nullptr;

    QString m_local_db_pth_str;
    QString m_local_db_name_str;
    bool m_local_db_ready = false;

    QSqlDatabase m_local_db, m_remote_db;

    QString create_expt_tbl_cmd();
    QString create_objects_tbl_cmd();
    QString create_devices_tbl_cmd();
    QString create_datum_tbl_cmd();
    QString create_datum_view_cmd();

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
    bool create_tbls_and_views();

public:
    SkinDatabase()
    {
        m_tbl_create_fac.insert(m_tbl_expts_str, &SkinDatabase::expt_tbl_col_def);
        m_tbl_create_fac.insert(m_tbl_objects_str, &SkinDatabase::objects_tbl_col_def);
        m_tbl_create_fac.insert(m_tbl_devices_str, &SkinDatabase::devices_tbl_col_def);
        m_tbl_create_fac.insert(m_tbl_datum_str, &SkinDatabase::datum_tbl_col_def);
        m_view_create_fac.insert(m_view_datum_str, &SkinDatabase::datum_view_col_def);

        /*
        QStringList dl = QSqlDatabase::drivers();
        QString ds = dl.join(QString("\n"));
        QString mysql_str = QString("\nmysql:%1").arg((int)(QSqlDatabase::isDriverAvailable("QMYSQL")));
        QString sqlite_str = QString("sqlite:%1").arg((int)(QSqlDatabase::isDriverAvailable("QSQLITE")));
        QString sts = ds + mysql_str + "\n" + sqlite_str;
        QMessageBox::information(nullptr, "", sts);
        */
        /*
         PGconn *con = PQconnectdb("host=server user=bart password=simpson dbname=springfield");
         QPSQLDriver *drv = new QPSQLDriver(con);
         QSqlDatabase db = QSqlDatabase::addDatabase(drv); // becomes the new default connection
         QSqlQuery query;
         */
    }

    SkinDatabase(bool use_remote_db)
    {
    }

    ~SkinDatabase()
    {}

    void set_remote_db_info(setting_db_info_t * db_info);
    void set_local_db_pth_str(QString str);
    void store_these_info(db_info_intf_t &info);
    bool prepare_local_db();
};

#endif // SQLDB_WORKS_H
