#include <QSqlQuery>
#include <QSqlError>
#include <QStringEncoder>
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

#define _VIEW_DATUM_COLS_ \
        (m_col_obj_id_str + "," + m_col_skin_type_str + "," + m_col_pos_str + ","\
        + m_col_lambda_str+ "," + m_col_data_str + "," + m_col_date_str + ","\
        + m_col_time_str + "," + m_col_dev_id_str + "," + m_col_expt_id_str)
#define _CREATE_VIEW_DATUM_CMD_ \
    (QString("CREATE VIEW IF NOT EXISTS ") + m_view_datum_str + " AS"\
        + " SELECT "\
        + m_tbl_datum_str + "." + m_col_obj_id_str + ","\
        + m_tbl_objects_str + "." + m_col_skin_type_str + ","\
        + m_tbl_datum_str + "." + m_col_pos_str + ","\
        + m_tbl_datum_str + "." + m_col_lambda_str+ ","\
        + m_tbl_datum_str + "." + m_col_data_str + ","\
        + m_tbl_datum_str + "." + m_col_date_str + ","\
        + m_tbl_datum_str + "." + m_col_time_str + ","\
        + m_tbl_datum_str + "." + m_col_dev_id_str + ","\
        + m_tbl_datum_str + "." + m_col_expt_id_str\
        + " FROM " + m_tbl_datum_str\
        + " LEFT JOIN " + m_tbl_objects_str\
        + " ON " + m_tbl_datum_str + "." + m_col_obj_id_str\
        + " = " + m_tbl_objects_str + "." + m_col_obj_id_str + ";")

/* macro works fine in single thread environment, but awkward in multi-thread
 * condition. so we change them to inline functions... */
/*info must be of type db_info_intf_t*/
/*
#define _INSERT_TBL_EXPT_CMD_(info) \
    (QString("INSERT INTO ") + m_tbl_expts_str + " VALUES "\
     + "(" + "\"" + (info).expt_id + "\"" + "," + "\"" + (info).expt_desc +"\"" + ","\
    + "\"" + (info).expt_date + "\"" + "," + "\"" + (info).expt_time +"\"" + ");")
*/
static inline QString _INSERT_TBL_EXPT_CMD_(SkinDatabase::db_info_intf_t &info)
{
    return
    (QString("INSERT INTO ") + m_tbl_expts_str + " VALUES "\
     + "(" + "\"" + (info).expt_id + "\"" + "," + "\"" + (info).expt_desc +"\"" + ","\
    + "\"" + (info).expt_date + "\"" + "," + "\"" + (info).expt_time +"\"" + ");");
}

static inline QString _FORCE_UPD_TBL_EXPT_CLAU_(SkinDatabase::db_info_intf_t &info)
{
    return
        (m_col_expt_id_str + "=" + "\"" + (info).expt_id + "\"" + ","\
        + m_col_desc_str + "=" + "\"" + (info).expt_desc + "\"" + ","\
        + m_col_date_str + "=" + "\"" + (info).expt_date +"\"" + ","\
        + m_col_time_str + "="  + "\"" + (info).expt_time  + "\"");
}

/*info must be of type db_info_intf_t*/
static inline QString _INSERT_TBL_OBJECTS_CMD_(SkinDatabase::db_info_intf_t &info)
{
    return
    (QString("INSERT INTO ") + m_tbl_objects_str + " VALUES "\
     + "(" + "\"" + (info).obj_id + "\"" + "," + "\"" + (info).skin_type + "\"" + ","\
     + "\"" + (info).obj_desc + "\"" + ");");
}
 static inline QString _FORCE_UPD_TBL_OBJECTS_CLAU_(SkinDatabase::db_info_intf_t &info)
{
    return
    (m_col_obj_id_str + "=" + "\"" + (info).obj_id  + "\"" + ","\
    + m_col_skin_type_str + "=" + "\"" + (info).skin_type + "\"" + ","\
    + m_col_desc_str + "=" + "\"" + (info).obj_desc + "\"");
}

/*info must be of type db_info_intf_t*/
static inline QString _INSERT_TBL_DEVICES_CMD_(SkinDatabase::db_info_intf_t &info)
{
    return
    (QString("INSERT INTO ") + m_tbl_devices_str + " VALUES "\
     + "("  + "\"" + (info).dev_id + "\"" + "," + "\"" + (info).dev_addr + "\"" + ","\
     + "\"" + (info).dev_desc + "\"" + ");");
}
static inline QString _FORCE_UPD_TBL_DEVICES_CLAU_(SkinDatabase::db_info_intf_t &info)
{
    return
        (m_col_dev_id_str + "=" + "\"" + (info).dev_id + "\"" + ","\
        + m_col_dev_addr_str + "=" + "\"" + (info).dev_addr + "\"" + ","\
        + m_col_desc_str + "=" + "\"" + (info).dev_desc + "\"");
}

/*info must be of type db_info_intf_t*/
static inline QString _INSERT_TBL_DATUM_CMD_(SkinDatabase::db_info_intf_t &info, int i)
{
    return
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
     + ");");
}

/*This is used for writing local csv file. info must be of type db_info_intf_t*/
static inline QString _INSERT_VIEW_DATUM_(SkinDatabase::db_info_intf_t &info, int i)
{
    return
     ((info).obj_id+ "," +  (info).skin_type + "," + (info).pos+ ","\
     + QString::number((info).lambda_data[i].lambda) + ","\
     + QString::number((info).lambda_data[i].data) + ","\
    + (info).rec_date + "," + (info).rec_time + ","\
    + (info).dev_id + ","+ (info).expt_id);
}

#define _SQLITE_INSERT_UPDATE_CLAU_ QString("ON CONFLICT DO UPDATE SET")
#define _MYSQL_INSERT_UPDATE_CLAU_  QString("ON DUPLICATE KEY UPDATE")

typedef struct
{
    QString tv_name;
    QString cmd_str;
    QString force_upd_clau;
    bool changed;
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
    if(m_local_csv_ready)
    {
        m_local_csv_f.close();
    }
}

void SkinDatabase::set_remote_db_info(setting_db_info_t * db_info)
{
    m_remote_db_info = db_info;
}
void SkinDatabase::set_local_store_pth_str(QString db, QString csv)
{
    m_local_db_pth_str = db;
    m_local_csv_pth_str = csv;
}
bool SkinDatabase::prepare_local_csv()
{
    m_local_csv_name_str = m_db_name_str + ".csv";
    QString fpn = m_local_csv_pth_str + "/" + m_local_csv_name_str;
    if(!m_local_csv_ready)
    {
        bool new_file = false;;
        m_local_csv_f.setFileName(fpn);
        if(m_local_csv_f.exists())
        {
            m_local_csv_ready = m_local_csv_f.open(QFile::Append);
            if(m_local_csv_ready && (m_local_csv_f.size() == 0))
            {
                new_file = true;
            }
        }
        else
        {
            new_file = true;
            m_local_csv_ready = m_local_csv_f.open(QFile::WriteOnly);
        }
        if(m_local_csv_ready && new_file)
        {
            QString header = _VIEW_DATUM_COLS_ + "\n";
            m_local_csv_f.write(header.toUtf8());
        }
    }
    if(!m_local_csv_ready)
    {
        DIY_LOG(LOG_LEVEL::LOG_ERROR, "Open/Create file %ls error!", fpn.utf16());
    }
    return m_local_csv_ready;
}

bool SkinDatabase::prepare_local_db()
{
    if(!m_local_db_ready)
    {
        m_local_db = QSqlDatabase::addDatabase(QString("QSQLITE"));

        /*
        m_local_db_name_str = m_db_name_str + "_" + m_intf.expt_date
                            + "-" + m_intf.expt_time;
        */
        m_local_db_name_str = m_db_name_str;
        m_local_db_name_str.replace(":","");
        QString fpn = m_local_db_pth_str + "/" + m_local_db_name_str;
        m_local_db.setDatabaseName(fpn);
        m_local_db_ready = m_local_db.open();
        if(!m_local_db_ready)
        {
           DIY_LOG(LOG_LEVEL::LOG_ERROR, "Local db %ls open error!", fpn.utf16());
           return m_local_db_ready;
        }
        m_local_db_ready = create_tbls_and_views(m_local_db);
        if(!m_local_db_ready)
        {
            m_local_db.close();
            return m_local_db_ready;
        }
    }
    return m_local_db_ready;
}

/*
 * This functon may be invoked from multi threads, so it should be thread safe.
*/
bool SkinDatabase::create_tbls_and_views(QSqlDatabase &qdb)
{
    QSqlQuery query(qdb);
    bool ret = true;
    QString name, cmd;
    tv_name_cmd_map_t create_tv_cmds[] =
    {
       {m_tbl_expts_str, _CREATE_TBL_EXPTS_CMD_, "", true},
       {m_tbl_objects_str, _CREATE_TBL_OBJECTS_CMD_, "", true},
       {m_tbl_devices_str, _CREATE_TBL_DEVICES_CMD_, "", true},
       {m_tbl_datum_str, _CREATE_TBL_DATUM_CMD_, "", true},
       {m_view_datum_str, _CREATE_VIEW_DATUM_CMD_, "", true},
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

bool SkinDatabase::write_local_db(QSqlDatabase &qdb, db_info_intf_t &intf)
{
    return write_db(qdb, intf, SkinDatabase::LOCAL);
}

/*
 * This functon may be invoked from multi threads, so it should be thread safe.
*/
bool SkinDatabase::write_db(QSqlDatabase &qdb, db_info_intf_t &intf, db_pos_t /*db_pos*/)
{
    QSqlQuery query(qdb);
    QSqlError sql_err;
    QString name, cmd;
    bool ret;

    tv_name_cmd_map_t insert_tbl_cmds[] =
    {
       {m_tbl_expts_str,
            _INSERT_TBL_EXPT_CMD_(intf), _FORCE_UPD_TBL_EXPT_CLAU_(intf),
            intf.expt_changed},
       {m_tbl_objects_str,
            _INSERT_TBL_OBJECTS_CMD_(intf), _FORCE_UPD_TBL_OBJECTS_CLAU_(intf),
            intf.obj_changed},
       {m_tbl_devices_str,
            _INSERT_TBL_DEVICES_CMD_(intf), _FORCE_UPD_TBL_DEVICES_CLAU_(intf),
            intf.dev_changed},
       {m_tbl_datum_str, "", "", true},
    };

    int idx = 0, d_idx = 0;
    while(idx < DIY_SIZE_OF_ARRAY(insert_tbl_cmds))
    {
        name = insert_tbl_cmds[idx].tv_name;
        if(m_tbl_datum_str == name)
        {
            while(d_idx < intf.lambda_data.count())
            {
                cmd =  _INSERT_TBL_DATUM_CMD_(intf, d_idx);
                ret = query.exec(cmd);
                if(!ret)
                {
                    sql_err = query.lastError();
                    DIY_LOG(LOG_LEVEL::LOG_ERROR,
                            "Insert table %ls fail, stop!!!\n"
                            "Cmd:\n%ls\nError:type %d, code %ls,%ls",
                           name.utf16(), cmd.utf16(), (int)sql_err.type(),
                           sql_err.nativeErrorCode().utf16(), sql_err.text().utf16());
                    return false;
                }
                ++d_idx;
            }
            ++idx;
            continue;
        }
        if(insert_tbl_cmds[idx].changed)
        {
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
                return false;
            }
        }
        ++idx;
    }
    return true;
}

bool SkinDatabase::write_local_csv(db_info_intf_t &intf)
{
    /* Headers are already written in prepare function. Here we just write data.*/
    if(m_local_csv_ready)
    {
        bool ret = true;
        int d_idx = 0;
        qint64 wd;
        QString line;
        /*Use this encoder so that the csv file can be directely displayed in excel.*/
        QStringEncoder enc = QStringEncoder(QStringEncoder::System);
        while(d_idx < intf.lambda_data.count())
        {
            line = _INSERT_VIEW_DATUM_(intf, d_idx) + "\n";
            //wd = m_local_csv_f.write(line.toUtf8());
            wd = m_local_csv_f.write(enc(line));
            if(wd < 0)
            {
                DIY_LOG(LOG_LEVEL::LOG_ERROR, "Write local csv fail, stop!!!");
                ret = false;
                break;
            }
            ++d_idx;
        }
        return ret;
    }
    else
    {
        return false;
    }
}

bool SkinDatabase::store_these_info(db_info_intf_t &info)
{
    bool ret = false, ret2 = false;
    m_intf = info;
    if(!m_local_db_ready)
    {
        m_local_db_ready = prepare_local_db();
    }
    if(m_local_db_ready)
    {
        ret = write_local_db(m_local_db, m_intf);
        m_local_db.close();
    }
    m_local_db_ready = false;

    if(!m_local_csv_ready)
    {
        m_local_csv_ready = prepare_local_csv();
    }
    if(m_local_csv_ready)
    {
        ret2 = write_local_csv(m_intf);
        m_local_csv_f.close();
    }
    m_local_csv_ready = false;

    return ret && ret2;
}
