#ifndef SQLDB_WORKS_H
#define SQLDB_WORKS_H

#include <QString>
#include <QStringList>
#include <QSqlDatabase>
#include <QMessageBox>

#include "diy_common_tool.h"
/*
 * local db: use sqlite to store data into a local db file.
 * remote db: use mysql server.
*/
class SkinDatabase
{
private:
    const QString m_db_name_str{"detector"};
    const QString m_tbl_expts_name_str{"experiments"};
    const QString m_col_expt_id_name_str{"expt_id"};
    const QString m_col_desc_name_str{"desc"};
    const QString m_col_date_name_str{"date"};
    const QString m_tbl_objects_name{"objects"};
    const QString m_col_obj_id_name_str{"obj_id"};
    const QString m_col_skin_type_name_str{"skin_type"};
    const QString m_tbl_devices_name_str{"devices"};
    const QString m_col_dev_id_name_str{"dev_id"};
    const QString m_col_dev_addr_name_str{"dev_addr"};
    const QString m_tbl_datum_name_str{"datum"};
    const QString m_col_pos_name_str{"pos"};
    const QString m_col_lambda_name_str{"lambda"};
    const QString m_col_data_name_str{"data"};
    const QString m_col_time_name_str{"time"};
    const QString m_col_rec_id_name_str{"rec_id"};
    const QString m_view_datum_name_str{"datum_view"};

    setting_db_info_t *m_remote_db_info = nullptr;
    QString m_local_db_pth_str;

    QSqlDatabase m_local_db, m_remote_db;
public:
    SkinDatabase()
    {
        /*
        QStringList dl = QSqlDatabase::drivers();
        QString ds = dl.join(QString("\n"));
        QString mysql_str = QString("\nmysql:%1").arg((int)(QSqlDatabase::isDriverAvailable("QMYSQL")));
        QString sqlite_str = QString("sqlite:%1").arg((int)(QSqlDatabase::isDriverAvailable("QSQLITE")));
        QString sts = ds + mysql_str + "\n" + sqlite_str;
        QMessageBox::information(nullptr, "", sts);
        SkinDatabase::m_local_db = QSqlDatabase::addDatabase(QString("QSQLITE"));
        SkinDatabase::m_remote_db = QSqlDatabase::addDatabase(QString("QMYSQL"));

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
};

#endif // SQLDB_WORKS_H
