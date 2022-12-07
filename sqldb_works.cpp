#include <QSqlQuery>
#include <QSqlError>
#include "sqldb_works.h"
#include "logger.h"

/*
 * Begin: Som sqlite defines.
 * Refer to https://www.sqlite.org/rescode.html#constraint_primarykey
 * We do not include other header files, and just define those things (e.g. result codes)
 * we handle here.
 * This is just for simplicity, and in future we may use those header files...
*/
#define SQLITE_CONSTRAINT_PRIMARYKEY 1555
#define SQLITE_CONSTRAINT_UNIQUE 2067
/*
 * End.
*/

#define m_db_name_str QString("detector")
#define m_tbl_expts_str QString("experiments")
#define m_col_expt_id_str QString("expt_id")
#define m_col_desc_str QString("desc")
#define m_col_date_str QString("date")
#define m_tbl_objects_str QString("objects")
#define m_col_obj_id_str QString("obj_id")
#define m_col_skin_type_str QString("skin_type")
#define m_tbl_devices_str QString("devices")
#define m_col_dev_id_str QString("dev_id")
#define m_col_dev_addr_str QString("dev_addr")
#define m_tbl_datum_str QString("datum")
#define m_col_pos_str QString("pos")
#define m_col_lambda_str QString("lambda")
#define m_col_data_str QString("data")
#define m_col_time_str QString("time")
#define m_col_rec_id_str QString("rec_id")
#define m_view_datum_str QString("datum_view")

#define _TBL_EXPTS_COLS_ \
        (m_col_expt_id_str + "," + m_col_desc_str + ","\
        + m_col_date_str + "," + m_col_time_str)
#define _CREATE_TBL_EXPTS_CMD_ \
     (QString("CREATE TABLE IF NOT EXISTS ") + m_tbl_expts_str + " ("\
        + m_col_expt_id_str + QString(" TEXT,") + m_col_desc_str + QString(" TEXT,")\
        + m_col_date_str + QString(" TEXT,") + m_col_time_str + QString(" TEXT,")\
        + QString("PRIMARY KEY(") + m_col_expt_id_str + QString(")") + ");")

#define _TBL_OBJECTS_COLS_ \
        (m_col_obj_id_str + "," + m_col_skin_type_str + "," + m_col_desc_str)
#define _CREATE_TBL_OBJECTS_CMD_ \
     (QString("CREATE TABLE IF NOT EXISTS ") + m_tbl_objects_str + " ("\
        + m_col_obj_id_str + QString(" TEXT,") + m_col_skin_type_str + QString(" TEXT,")\
        + m_col_desc_str + QString(" TEXT,")\
        + QString("PRIMARY KEY(") + m_col_obj_id_str + QString(")") + QString(");"))

#define _TBL_DEVICES_COLS_ \
        (m_col_dev_id_str + "," + m_col_dev_addr_str + "," + m_col_desc_str)
#define _CREATE_TBL_DEVICES_CMD_ \
     (QString("CREATE TABLE IF NOT EXISTS ") + m_tbl_devices_str + " ("\
        + m_col_dev_id_str + QString(" TEXT,") + m_col_dev_addr_str + QString(" TEXT,")\
        + m_col_desc_str + QString(" TEXT,")\
        + QString("PRIMARY KEY(") + m_col_dev_id_str + QString(")")+ QString(");"))

/*No rec_id.*/
#define _TBL_DATUM_COLS_ \
        (m_col_obj_id_str + "," + m_col_pos_str + ","\
        + m_col_lambda_str + ","\
        + m_col_data_str + ","\
        + m_col_date_str + "," + m_col_time_str + ","\
        + m_col_dev_id_str + "," + m_col_expt_id_str)
#define _CREATE_TBL_DATUM_CMD_ \
     (QString("CREATE TABLE IF NOT EXISTS ") + m_tbl_datum_str + " ("\
        + m_col_obj_id_str + QString(" TEXT,") + m_col_pos_str + QString(" TEXT,")\
        + m_col_lambda_str + QString(" UNSIGNED INT,")\
        + m_col_data_str + QString(" UNSIGNED BIG INT,")\
        + m_col_date_str + QString(" TEXT,") + m_col_time_str + QString(" TEXT,")\
        + m_col_dev_id_str + QString(" TEXT,") + m_col_expt_id_str + QString(" TEXT,")\
        + m_col_rec_id_str + QString(" INTEGER,")\
        + QString("PRIMARY KEY(") + m_col_rec_id_str + QString(")")+ QString(");"))

#define _CREATE_VIEW_DATUM_CMD_ \
    (QString("CREATE VIEW IF NOT EXISTS ") + m_view_datum_str + " AS"\
        + " SELECT "\
        + m_tbl_datum_str + "." + m_col_dev_id_str + ","\
        + m_tbl_datum_str + "." + m_col_obj_id_str + ","\
        + m_tbl_objects_str + "." + m_col_skin_type_str + ","\
        + m_tbl_datum_str + "." + m_col_pos_str + ","\
        + m_tbl_datum_str + "." + m_col_lambda_str+ ","\
        + m_tbl_datum_str + "." + m_col_data_str + ","\
        + m_tbl_datum_str + "." + m_col_date_str + ","\
        + m_tbl_datum_str + "." + m_col_time_str + ","\
        + m_tbl_datum_str + "." + m_col_expt_id_str\
        + " FROM " + m_tbl_datum_str\
        + " LEFT JOIN " + m_tbl_objects_str\
        + " ON " + m_tbl_datum_str + "." + m_col_obj_id_str\
        + " = " + m_tbl_objects_str + "." + m_col_obj_id_str + ";")

/*info must be of type db_info_intf_t*/
#define _INSERT_TBL_EXPT_CMD_(info) \
    (QString("INSERT INTO ") + m_tbl_expts_str + " VALUES "\
     + "(" + "\"" + (info).expt_id + "\"" + "," + "\"" + (info).expt_desc +"\"" + ","\
    + "\"" + (info).expt_date + "\"" + "," + "\"" + (info).expt_time +"\"" + ");")
#define _FORCE_UPD_TBL_EXPT_CLAU_(info)\
        (m_col_expt_id_str + "=" + "\"" + (info).expt_id + "\"" + ","\
        + m_col_desc_str + "=" + "\"" + (info).expt_desc + "\"" + ","\
        + m_col_date_str + "=" + "\"" + (info).expt_date +"\"" + ","\
        + m_col_time_str + "="  + "\"" + (info).expt_time  + "\"")

/*info must be of type db_info_intf_t*/
#define _INSERT_TBL_OBJECTS_CMD_(info) \
    (QString("INSERT INTO ") + m_tbl_objects_str + " VALUES "\
     + "(" + "\"" + (info).obj_id + "\"" + "," + "\"" + (info).skin_type + "\"" + ","\
     + "\"" + (info).obj_desc + "\"" + ");")
#define _FORCE_UPD_TBL_OBJECTS_CLAU_(info)\
    (m_col_obj_id_str + "=" + "\"" + (info).obj_id  + "\"" + ","\
    + m_col_skin_type_str + "=" + "\"" + (info).skin_type + "\"" + ","\
    + m_col_desc_str + "=" + "\"" + (info).obj_desc + "\"")

/*info must be of type db_info_intf_t*/
#define _INSERT_TBL_DEVICES_CMD_(info) \
    (QString("INSERT INTO ") + m_tbl_devices_str + " VALUES "\
     + "("  + "\"" + (info).dev_id + "\"" + "," + "\"" + (info).dev_addr + "\"" + ","\
     + "\"" + (info).dev_desc + "\"" + ");")
#define _FORCE_UPD_TBL_DEVICES_CLAU_(info)\
        (m_col_dev_id_str + "=" + "\"" + (info).dev_id + "\"" + ","\
        + m_col_dev_addr_str + "=" + "\"" + (info).dev_addr + "\"" + ","\
        + m_col_desc_str + "=" + "\"" + (info).dev_desc + "\"")

/*info must be of type db_info_intf_t*/
#define _INSERT_TBL_DATUM_CMD_(info, i) \
    (QString("INSERT INTO ") + m_tbl_datum_str\
     + " ("\
     + m_col_obj_id_str + "," + m_col_pos_str + ","\
     + m_col_lambda_str + "," + m_col_data_str + ","\
     + m_col_date_str + "," + m_col_time_str + ","\
     + m_col_dev_id_str + "," + m_col_expt_id_str\
     + ") "\
     + "VALUES "\
     + "(" + "\"" + (info).obj_id + "\"" + "," + "\"" + (info).pos + "\"" + ","\
     + QString::number((info).lambda_data[i].lambda) + ","\
     + QString::number((info).lambda_data[i].data) + ","\
     + "\"" + (info).rec_date + "\"" + ","  + "\"" + (info).rec_time + "\"" + ","\
     + "\"" + (info).dev_id + "\"" + "," + "\"" + (info).expt_id + "\""\
     + ");")

#define _SQLITE_INSERT_UPDATE_CLAU_ QString("ON CONFLICT DO UPDATE SET")
#define _MYSQL_INSERT_UPDATE_CLAU_  QString("ON DUPLICATE KEY UPDATE")

typedef struct
{
    QString tv_name;
    QString cmd_str;
    QString force_upd_clau;
}tv_name_cmd_map_t;

SkinDatabase::SkinDatabase()
{
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
SkinDatabase::~SkinDatabase()
{
    m_intf.lambda_data.clear();
    if(m_local_db_ready)
    {
        m_local_db.close();
    }
}

void SkinDatabase::set_remote_db_info(setting_db_info_t * db_info)
{
    m_remote_db_info = db_info;
}
void SkinDatabase::set_local_db_pth_str(QString str)
{
    m_local_db_pth_str = str;
}
bool SkinDatabase::prepare_local_db()
{
    if(!m_local_db_ready)
    {
        m_local_db = QSqlDatabase::addDatabase(QString("QSQLITE"));

        m_local_db_name_str = m_db_name_str + "_" + m_intf.expt_date
                            + "-" + m_intf.expt_time;
        m_local_db_name_str.replace(":","");
        QString fpn = m_local_db_pth_str + "/" + m_local_db_name_str;
        m_local_db.setDatabaseName(fpn);
        m_local_db_ready = m_local_db.open();
        if(!m_local_db_ready)
        {
           DIY_LOG(LOG_LEVEL::LOG_ERROR, "Local db %ls open error!", fpn.utf16());
           return m_local_db_ready;
        }
        m_local_db_ready = create_tbls_and_views();
        if(!m_local_db_ready)
        {
            return m_local_db_ready;
        }
    }
    return m_local_db_ready;
}

bool SkinDatabase::create_tbls_and_views()
{
    QSqlQuery query(m_local_db);
    bool ret = true;
    QString name, cmd;
    tv_name_cmd_map_t create_tv_cmds[] =
    {
       {m_tbl_expts_str, _CREATE_TBL_EXPTS_CMD_, ""},
       {m_tbl_objects_str, _CREATE_TBL_OBJECTS_CMD_, ""},
       {m_tbl_devices_str, _CREATE_TBL_DEVICES_CMD_, ""},
       {m_tbl_datum_str, _CREATE_TBL_DATUM_CMD_, ""},
       {m_view_datum_str, _CREATE_VIEW_DATUM_CMD_, ""},
    };

    /*Create TABLE and VIEW*/
    int idx = 0;
    while(idx < DIY_SIZE_OF_ARRAY(create_tv_cmds))
    {
        name = create_tv_cmds[idx].tv_name;
        cmd = create_tv_cmds[idx].cmd_str;
        ret = query.exec(cmd);
        if(!ret)
        {
           DIY_LOG(LOG_LEVEL::LOG_ERROR, "Create table/view %ls fail! Cmd:\n%ls",
                   name.utf16(), cmd.utf16());
           return ret;
        }
        ++idx;
    }

    return ret;
}

void SkinDatabase::store_these_info(db_info_intf_t &info)
{
    m_intf = info;
    if(!prepare_local_db())
    {
        return;
    }

    QSqlQuery query(m_local_db);
    QSqlError sql_err;
    QString name, cmd;
    bool ret;
    tv_name_cmd_map_t insert_tbl_cmds[] =
    {
       {m_tbl_expts_str, _INSERT_TBL_EXPT_CMD_(m_intf),
                         _FORCE_UPD_TBL_EXPT_CLAU_(m_intf)},
       {m_tbl_objects_str, _INSERT_TBL_OBJECTS_CMD_(m_intf),
                           _FORCE_UPD_TBL_OBJECTS_CLAU_(m_intf)},
       {m_tbl_devices_str, _INSERT_TBL_DEVICES_CMD_(m_intf),
                           _FORCE_UPD_TBL_DEVICES_CLAU_(m_intf)},
       {m_tbl_datum_str, "", ""},
    };

    int idx = 0, d_idx = 0;
    while(idx < DIY_SIZE_OF_ARRAY(insert_tbl_cmds))
    {
        name = insert_tbl_cmds[idx].tv_name;
        if(m_tbl_datum_str == name)
        {
            while(d_idx < m_intf.lambda_data.count())
            {
                cmd =  _INSERT_TBL_DATUM_CMD_(m_intf, d_idx);
                ret = query.exec(cmd);
                if(!ret)
                {
                    sql_err = query.lastError();
                    DIY_LOG(LOG_LEVEL::LOG_ERROR,
                            "Insert table %ls fail, stop!!!\n"
                            "Cmd:\n%ls\nError:type %d, code %ls,%ls",
                           name.utf16(), cmd.utf16(), (int)sql_err.type(),
                           sql_err.nativeErrorCode().utf16(), sql_err.text().utf16());
                    return;
                }
                ++d_idx;
            }
            ++idx;
            continue;
        }
        cmd = insert_tbl_cmds[idx].cmd_str;
        ret = query.exec(cmd);
        if(!ret)
        {
            sql_err = query.lastError();
            quint32 err_code = sql_err.nativeErrorCode().toUInt();
            if(err_code == SQLITE_CONSTRAINT_PRIMARYKEY
                    || err_code == SQLITE_CONSTRAINT_UNIQUE)
            {
                DIY_LOG(LOG_LEVEL::LOG_INFO, "inser duplicate, now update");
                /*update*/
                cmd.remove(";");
                cmd += " " + _SQLITE_INSERT_UPDATE_CLAU_ + " "
                        + insert_tbl_cmds[idx].force_upd_clau + ";";
                ret = query.exec(cmd);
                if(ret)
                {
                    ++idx;
                    continue;
                }
            }
            DIY_LOG(LOG_LEVEL::LOG_ERROR,
                    "Insert table %ls fail, stop!!!\n"
                    "Cmd:\n%ls\nError:type %d, code %ls,%ls",
                   name.utf16(), cmd.utf16(), (int)sql_err.type(),
                   sql_err.nativeErrorCode().utf16(), sql_err.text().utf16());
            return;
        }
        ++idx;
    }
}
