#include <QSqlQuery>
#include <QSqlError>
#include <QStringEncoder>
#include "sqldb_remote_worker.h"
#include "sqldb_works.h"
#include "logger.h"
#include "diy_common_tool.h"

#define LOCAL_DB_CONN_NAME QString("local_sqlite_conntection")
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

/*
 * Begin: Mysql defines.
 * Refer to https://dev.mysql.com/doc/mysql-errors/5.7/en/server-error-reference.html#error_er_no_such_table
*/
#define MYSQL_ER_DUP_KEY 1022
#define MYSQL_ER_DUP_ENTRY 1062
/*
 * End.
*/

#define m_db_name_str QString("detector")
#define m_tbl_expts_str QString("experiments")
#define m_col_expt_id_str QString("expt_id")
#define m_col_desc_str QString("descpt") //"desc" is key word in my sql and can't be column.
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

#define _SQLITE_INSERT_UPDATE_CLAU_ QString("ON CONFLICT DO UPDATE SET")
#define _MYSQL_INSERT_UPDATE_CLAU_  QString("ON DUPLICATE KEY UPDATE")

#define _TBL_EXPTS_COLS_ \
        (m_col_expt_id_str + "," + m_col_desc_str + ","\
        + m_col_date_str + "," + m_col_time_str)
/*sqlite and mysql differs slightly in syntax. so we change the cmd generator
 * from macro to function to increase flexibility.
*/
#define TEXT_KEY_LEN 255 //Mysql needs a length for TEXT key.
static inline QString _CREATE_TBL_EXPTS_CMD_(SkinDatabase::db_type_t db_t)
{
    QString cmd;
    cmd =
     (QString("CREATE TABLE IF NOT EXISTS ") + m_tbl_expts_str + " ("\
        + m_col_expt_id_str + QString(" TEXT,") + m_col_desc_str + QString(" TEXT,")\
        + m_col_date_str + QString(" TEXT,") + m_col_time_str + QString(" TEXT,")\
        + QString("PRIMARY KEY(") +  m_col_expt_id_str);
    if(SkinDatabase::DB_SQLITE == db_t)
    {
        cmd += QString("));");
    }
    else //assuming DB_MYSQL
    {
        cmd += QString("(%1)));").arg(TEXT_KEY_LEN);
    }
    return cmd;
}

#define _TBL_OBJECTS_COLS_ \
        (m_col_obj_id_str + "," + m_col_skin_type_str + "," + m_col_desc_str)
static inline QString _CREATE_TBL_OBJECTS_CMD_(SkinDatabase::db_type_t db_t)
{
    QString cmd;
    cmd =
     (QString("CREATE TABLE IF NOT EXISTS ") + m_tbl_objects_str + " ("\
        + m_col_obj_id_str + QString(" TEXT,") + m_col_skin_type_str + QString(" TEXT,")\
        + m_col_desc_str + QString(" TEXT,")\
        + QString("PRIMARY KEY(") + m_col_obj_id_str);
    if(SkinDatabase::DB_SQLITE == db_t)
    {
         cmd += QString("));");
    }
    else //assuming DB_MYSQL
    {
        cmd += QString("(%1)));").arg(TEXT_KEY_LEN);
    }
    return cmd;
}

#define _TBL_DEVICES_COLS_ \
        (m_col_dev_id_str + "," + m_col_dev_addr_str + "," + m_col_desc_str)
static inline QString _CREATE_TBL_DEVICES_CMD_(SkinDatabase::db_type_t db_t)
{
    QString cmd;
    cmd =
         (QString("CREATE TABLE IF NOT EXISTS ") + m_tbl_devices_str + " ("\
        + m_col_dev_id_str + QString(" TEXT,") + m_col_dev_addr_str + QString(" TEXT,")\
        + m_col_desc_str + QString(" TEXT,")\
        + QString("PRIMARY KEY(") + m_col_dev_id_str);
    if(SkinDatabase::DB_SQLITE == db_t)
    {
         cmd += QString("));");
    }
    else //assuming DB_MYSQL
    {
        cmd += QString("(%1)));").arg(TEXT_KEY_LEN);
    }
    return cmd;
}

/*No rec_id.*/
#define _TBL_DATUM_COLS_ \
        (m_col_obj_id_str + "," + m_col_pos_str + ","\
        + m_col_lambda_str + ","\
        + m_col_data_str + ","\
        + m_col_date_str + "," + m_col_time_str + ","\
        + m_col_dev_id_str + "," + m_col_expt_id_str)
static inline QString _CREATE_TBL_DATUM_CMD_(SkinDatabase::db_type_t db_t)
{
    QString cmd;
    cmd =
     (QString("CREATE TABLE IF NOT EXISTS ") + m_tbl_datum_str + " ("\
        + m_col_obj_id_str + QString(" TEXT,") + m_col_pos_str + QString(" TEXT,")\
        + m_col_lambda_str);
    if(SkinDatabase::DB_SQLITE == db_t)
    {
        cmd += QString(" UNSIGNED INT,");
    }
    else //assuming DB_MYSQL
    {
        cmd += QString(" INT UNSIGNED,");
    }
    cmd  += m_col_data_str;
    if(SkinDatabase::DB_SQLITE == db_t)
    {
        cmd += QString(" UNSIGNED BIGINT,");
    }
    else //assuming DB_MYSQL
    {
        cmd += QString(" BIGINT UNSIGNED,");
    }
    cmd += m_col_date_str + QString(" TEXT,") + m_col_time_str + QString(" TEXT,")\
        + m_col_dev_id_str + QString(" TEXT,") + m_col_expt_id_str + QString(" TEXT,")\
        + m_col_rec_id_str + QString(" INTEGER");
    if(SkinDatabase::DB_MYSQL == db_t)
    {
        cmd += QString(" AUTO_INCREMENT");
    }
    cmd += QString(",");
    cmd += QString("PRIMARY KEY(") + m_col_rec_id_str + QString(")")+ QString(");");
    return cmd;
}

#define _VIEW_DATUM_COLS_ \
        (m_col_obj_id_str + "," + m_col_skin_type_str + "," + m_col_pos_str + ","\
        + m_col_lambda_str+ "," + m_col_data_str + "," + m_col_date_str + ","\
        + m_col_time_str + "," + m_col_dev_id_str + "," + m_col_expt_id_str)
static inline QString _CREATE_VIEW_DATUM_CMD_(SkinDatabase::db_type_t db_t)
{
    QString cmd;
    if(SkinDatabase::DB_SQLITE == db_t)
    {
        cmd = QString("CREATE VIEW IF NOT EXISTS ") + m_view_datum_str;
    }
    else //assuming DB_MYSQL
    {
        cmd = QString("CREATE OR REPLACE VIEW ") + m_view_datum_str;
    }

    cmd += (QString(" AS")\
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
        + " = " + m_tbl_objects_str + "." + m_col_obj_id_str + ";");
    return cmd;
}
/* macro works fine in single thread environment, but awkward in multi-thread
 * condition. so we change them to inline functions... */
/*info must be of type db_info_intf_t*/
/*
#define _INSERT_TBL_EXPTS_CMD_(info) \
    (QString("INSERT INTO ") + m_tbl_expts_str + " VALUES "\
     + "(" + "\"" + (info).expt_id + "\"" + "," + "\"" + (info).expt_desc +"\"" + ","\
    + "\"" + (info).expt_date + "\"" + "," + "\"" + (info).expt_time +"\"" + ");")
*/
static inline QString _INSERT_TBL_EXPTS_CMD_(SkinDatabase::db_info_intf_t &info,
                                            SkinDatabase::db_type_t /*db_t*/)
{
    return
    (QString("INSERT INTO ") + m_tbl_expts_str + " VALUES "\
     + "(" + "\"" + (info).expt_id + "\"" + "," + "\"" + (info).expt_desc +"\"" + ","\
    + "\"" + (info).expt_date + "\"" + "," + "\"" + (info).expt_time +"\"" + ");");
}

static inline QString _FORCE_UPD_TBL_EXPTS_CLAU_(SkinDatabase::db_info_intf_t &info,
                                                SkinDatabase::db_type_t db_t)
{
    QString c = (SkinDatabase::DB_SQLITE == db_t) ?
                _SQLITE_INSERT_UPDATE_CLAU_ : _MYSQL_INSERT_UPDATE_CLAU_;
    return
        c + " "
        + (m_col_expt_id_str + "=" + "\"" + (info).expt_id + "\"" + ","\
        + m_col_desc_str + "=" + "\"" + (info).expt_desc + "\"" + ","\
        + m_col_date_str + "=" + "\"" + (info).expt_date +"\"" + ","\
        + m_col_time_str + "="  + "\"" + (info).expt_time  + "\"");
}

/*info must be of type db_info_intf_t*/
static inline QString _INSERT_TBL_OBJECTS_CMD_(SkinDatabase::db_info_intf_t &info,
                                               SkinDatabase::db_type_t /*db_t*/)
{
    return
    (QString("INSERT INTO ") + m_tbl_objects_str + " VALUES "\
     + "(" + "\"" + (info).obj_id + "\"" + "," + "\"" + (info).skin_type + "\"" + ","\
     + "\"" + (info).obj_desc + "\"" + ");");
}
 static inline QString _FORCE_UPD_TBL_OBJECTS_CLAU_(SkinDatabase::db_info_intf_t &info,
                                                    SkinDatabase::db_type_t db_t)
{
    QString c = (SkinDatabase::DB_SQLITE == db_t) ?
                _SQLITE_INSERT_UPDATE_CLAU_ : _MYSQL_INSERT_UPDATE_CLAU_;
    return
      c + " "
    + (m_col_obj_id_str + "=" + "\"" + (info).obj_id  + "\"" + ","\
    + m_col_skin_type_str + "=" + "\"" + (info).skin_type + "\"" + ","\
    + m_col_desc_str + "=" + "\"" + (info).obj_desc + "\"");
}

/*info must be of type db_info_intf_t*/
static inline QString _INSERT_TBL_DEVICES_CMD_(SkinDatabase::db_info_intf_t &info,
                                               SkinDatabase::db_type_t /*db_t*/)
{
    return
    (QString("INSERT INTO ") + m_tbl_devices_str + " VALUES "\
     + "("  + "\"" + (info).dev_id + "\"" + "," + "\"" + (info).dev_addr + "\"" + ","\
     + "\"" + (info).dev_desc + "\"" + ");");
}
static inline QString _FORCE_UPD_TBL_DEVICES_CLAU_(SkinDatabase::db_info_intf_t &info,
                                                   SkinDatabase::db_type_t db_t)
{
    QString c = (SkinDatabase::DB_SQLITE == db_t) ?
                _SQLITE_INSERT_UPDATE_CLAU_ : _MYSQL_INSERT_UPDATE_CLAU_;
    return
        c + " "
        + (m_col_dev_id_str + "=" + "\"" + (info).dev_id + "\"" + ","\
        + m_col_dev_addr_str + "=" + "\"" + (info).dev_addr + "\"" + ","\
        + m_col_desc_str + "=" + "\"" + (info).dev_desc + "\"");
}

/*info must be of type db_info_intf_t*/
static inline QString _INSERT_TBL_DATUM_CMD_(SkinDatabase::db_info_intf_t &info,
                                             int i,
                                             SkinDatabase::db_type_t db_t)
{
    QString cmd;
    cmd = (QString("INSERT INTO ") + m_tbl_datum_str\
     + " ("\
     + m_col_obj_id_str + "," + m_col_pos_str + ","\
     + m_col_lambda_str + "," + m_col_data_str + ","\
     + m_col_date_str + "," + m_col_time_str + ","\
     + m_col_dev_id_str + "," + m_col_expt_id_str);
    if(SkinDatabase::DB_MYSQL == db_t)
    {
       cmd += QString(",") + m_col_rec_id_str;
    }
    cmd += QString(") ")\
     + "VALUES "\
     + "(" + "\"" + (info).obj_id + "\"" + "," + "\"" + (info).pos + "\"" + ","\
     + QString::number((info).lambda_data[i].lambda) + ","\
     + QString::number((info).lambda_data[i].data) + ","\
     + "\"" + (info).rec_date + "\"" + ","  + "\"" + (info).rec_time + "\"" + ","\
     + "\"" + (info).dev_id + "\"" + "," + "\"" + (info).expt_id + "\"";
    if(SkinDatabase::DB_MYSQL == db_t)
    {
        cmd += QString(", NULL");
    }
    cmd += QString(");");
    return cmd;
}

/*This is used for writing local csv file. info must be of type db_info_intf_t*/
static inline QString _INSERT_VIEW_DATUM_(SkinDatabase::db_info_intf_t &info,
                                          int i,
                                          SkinDatabase::db_type_t /*db_t*/)
{
    return
     ((info).obj_id+ "," +  (info).skin_type + "," + (info).pos+ ","\
     + QString::number((info).lambda_data[i].lambda) + ","\
     + QString::number((info).lambda_data[i].data) + ","\
    + (info).rec_date + "," + (info).rec_time + ","\
    + (info).dev_id + ","+ (info).expt_id);
}

typedef struct
{
    QString tv_name;
    QString cmd_str;
    bool changed;
    QString aux_cmd_str; //use case: insert rdb with cmd_str fail, try to insert safe ldb.
}tv_name_cmd_map_t;
/*
 * All dabase related work, including management of the thread in charge of
 * remote db, are implemented in this class. The life cycle of this class instance
 * is controlled by upper object (chat for this programm).
*/
SkinDatabase::SkinDatabase(setting_rdb_info_t * rdb_info)
{
    DIY_LOG(LOG_LEVEL::LOG_INFO, "+++++++SkinDatabase constructor in thread: %u",
            (quint64)(QThread::currentThreadId()));

    set_remote_db_info(rdb_info);
    if(rdb_info)
    {
        DIY_LOG(LOG_LEVEL::LOG_INFO, "Try to start a thread for remote db store.");

        SqlDbRemoteWorker *rdb_worker = nullptr;
        rdb_worker = new SqlDbRemoteWorker;
        if(nullptr == rdb_worker)
        {
            set_remote_db_info(nullptr);
            DIY_LOG(LOG_LEVEL::LOG_ERROR,
                    "New SqlDbRemoteWorker error!"
                    "So thread for remote db store cant' be created.");
        }
        else
        {
            rdb_worker->moveToThread(&m_rdb_thread);
            connect(&m_rdb_thread, &QThread::finished,
                    rdb_worker, &QObject::deleteLater);

            connect(this, &SkinDatabase::prepare_rdb_sig,
                    rdb_worker, &SqlDbRemoteWorker::prepare_rdb);
            connect(rdb_worker, &SqlDbRemoteWorker::remote_db_prepared_sig,
                    this, &SkinDatabase::rdb_prepare_ret_handler);

            connect(this, &SkinDatabase::write_rdb_sig,
                    rdb_worker, &SqlDbRemoteWorker::write_rdb);
            connect(rdb_worker, &SqlDbRemoteWorker::remote_db_write_done_sig,
                    this, &SkinDatabase::remote_db_write_done_handler);

            connect(this, &SkinDatabase::close_rdb_sig,
                    rdb_worker, &SqlDbRemoteWorker::close_rdb);

            connect(this, &SkinDatabase::upload_safe_ldb_sig,
                    rdb_worker, &SqlDbRemoteWorker::upload_safe_ldb_to_rdb);
            connect(rdb_worker, &SqlDbRemoteWorker::upload_safe_ldb_done_sig,
                    this, &SkinDatabase::upload_safe_ldb_done_handler);

            m_rdb_thread.start();
            DIY_LOG(LOG_LEVEL::LOG_INFO, "New thead for remote db store started.");
        }
    }
    else
    {
        DIY_LOG(LOG_LEVEL::LOG_INFO, "rdb_info null, no need to use remote db.");
    }

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
    DIY_LOG(LOG_LEVEL::LOG_INFO, "------SkinDatabase destructor in thread: %u",
            (quint64)(QThread::currentThreadId()));

    m_intf.lambda_data.clear();
    if(m_local_db_ready)
    {
        remove_qt_sqldb_conn(LOCAL_DB_CONN_NAME);
        m_local_db_ready = false;
    }
    if(m_local_csv_ready)
    {
        m_local_csv_f.close();
        m_local_csv_ready = false;
    }
    if(m_remote_db_info)
    {
        m_remote_db_info = nullptr;

        m_rdb_thread.quit();
        m_rdb_thread.wait();
        DIY_LOG(LOG_LEVEL::LOG_INFO, "quit remote db thread.");
    }
}

void SkinDatabase::set_remote_db_info(setting_rdb_info_t * db_info)
{
    close_dbs(SkinDatabase::DB_REMOTE);
    m_remote_db_info = db_info;
}

void SkinDatabase::set_safe_ldb_for_rdb_fpn(QString safe_ldb_dir, QString safe_ldb_file)
{
    emit close_rdb_sig(SkinDatabase::DB_SAFE_LDB);
    m_safe_ldb_dir_str = safe_ldb_dir;
    m_safe_ldb_file_str = safe_ldb_file;
}

void SkinDatabase::set_local_store_pth_str(QString db, QString csv)
{
    if(m_local_csv_ready)
    {
        m_local_csv_f.close();
        m_local_csv_ready = false;
    }
    m_local_csv_pth_str = csv;

    if(m_local_db_ready)
    {
        remove_qt_sqldb_conn(LOCAL_DB_CONN_NAME);
        m_local_db_ready = false;
    }
    m_local_db_pth_str = db;
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
        while(true)
        {
            QSqlError sql_err;
            QString sqlerr_str;
            QSqlDatabase local_db;

            local_db = QSqlDatabase::addDatabase(QString("QSQLITE"), LOCAL_DB_CONN_NAME);
            if(!local_db.isValid())
            {
                sql_err = local_db.lastError();
                sqlerr_str = SQL_LAST_ERR_STR(sql_err);
                DIY_LOG(LOG_LEVEL::LOG_ERROR, "addDatabase QSQLITE error!"
                                              "conntion name: %ls\n%ls",
                                              LOCAL_DB_CONN_NAME.utf16(),
                                              sqlerr_str.utf16());
                m_local_db_ready = false;
                break;
            }
            /*
            m_local_db_name_str = m_db_name_str + "_" + m_intf.expt_date
                                + "-" + m_intf.expt_time;
            */
            m_local_db_name_str = m_db_name_str + ".sqlite";
            m_local_db_name_str.replace(":","");
            QString fpn = m_local_db_pth_str + "/" + m_local_db_name_str;
            local_db.setDatabaseName(fpn);
            m_local_db_ready = local_db.open();
            if(!m_local_db_ready)
            {
                sql_err = local_db.lastError();
                sqlerr_str = SQL_LAST_ERR_STR(sql_err);
                DIY_LOG(LOG_LEVEL::LOG_ERROR, "Local db %ls open error!\n%ls",
                        fpn.utf16(), sqlerr_str.utf16());
                break;
            }
            m_local_db_ready = create_tbls_and_views(local_db, LOCAL, DB_SQLITE);
            if(!m_local_db_ready)
            {
                local_db.close();
                break;
            }

            break;
        }
        if(!m_local_db_ready)
        {
            QSqlDatabase::removeDatabase(LOCAL_DB_CONN_NAME);
        }
    }
    return m_local_db_ready;
}

/*
 * This functon may be invoked from multi threads, so it should be thread safe.
*/
bool SkinDatabase::create_tbls_and_views(QSqlDatabase &qdb,
                                         db_pos_t /*db_pos*/, db_type_t db_type)
{
    QSqlQuery query(qdb);
    bool ret = true;
    QString name, cmd;
    tv_name_cmd_map_t create_tv_cmds[] =
    {
       {m_tbl_expts_str, _CREATE_TBL_EXPTS_CMD_(db_type), true, ""},
       {m_tbl_objects_str, _CREATE_TBL_OBJECTS_CMD_(db_type), true, ""},
       {m_tbl_devices_str, _CREATE_TBL_DEVICES_CMD_(db_type), true, ""},
       {m_tbl_datum_str, _CREATE_TBL_DATUM_CMD_(db_type), true, ""},
       {m_view_datum_str, _CREATE_VIEW_DATUM_CMD_(db_type),  true, ""},
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
            QSqlError sql_err = query.lastError();
            QString sqlerr_str = SQL_LAST_ERR_STR(sql_err);
            DIY_LOG(LOG_LEVEL::LOG_ERROR,
                    "Create table/view %ls fail!\nCmd:%ls\n%ls",
                    name.utf16(), cmd.utf16(), sqlerr_str.utf16());
           return ret;
        }
        ++idx;
    }

    return ret;
}

/*
 * This functon may be invoked from multi threads, so it should be thread safe.
*/
bool SkinDatabase::write_local_db(QSqlDatabase &qdb, db_info_intf_t &intf,
                                  db_type_t db_type)
{
    return write_db(qdb, intf, SkinDatabase::LOCAL, db_type);
}

/* invoked from remote worker thread.
 * if prepared ok, the db is opened for use.
*/
bool SkinDatabase::prepare_safe_ldb(QString db_pth, QString db_name, QString tbl_name,
                                    QString safe_ldb_conn_name_str)
{
    if(!mkpth_if_not_exists(db_pth))
    {
        DIY_LOG(LOG_LEVEL::LOG_ERROR, "Create dir fail:%ls", db_pth.utf16());
        return false;
    }
    QMap<QString, QString> create_tbl_cmds =
    {
       {m_tbl_expts_str, _CREATE_TBL_EXPTS_CMD_(DB_SQLITE)},
       {m_tbl_objects_str, _CREATE_TBL_OBJECTS_CMD_(DB_SQLITE)},
       {m_tbl_devices_str, _CREATE_TBL_DEVICES_CMD_(DB_SQLITE)},
       {m_tbl_datum_str, _CREATE_TBL_DATUM_CMD_(DB_SQLITE)},
    };
    bool ret = true;
    while(true)
    {
        QSqlError sql_err;
        QString sqlerr_str;
        QSqlDatabase local_db;

        local_db = QSqlDatabase::database(safe_ldb_conn_name_str);
        if(!local_db.isValid())
        {
            local_db = QSqlDatabase::addDatabase(QString("QSQLITE"), safe_ldb_conn_name_str);
        }
        if(!local_db.isValid())
        {
            sql_err = local_db.lastError();
            sqlerr_str = SQL_LAST_ERR_STR(sql_err);
            DIY_LOG(LOG_LEVEL::LOG_ERROR, "addDatabase QSQLITE error!"
                                          "conntion name: %ls!\n%ls",
                                          safe_ldb_conn_name_str.utf16(),
                                          sqlerr_str.utf16());
            ret = false;
            break;
        }
        if(!local_db.isOpen())
        {
            local_db.setDatabaseName(db_pth + "/" + db_name);
            ret = local_db.open();
        }
        if(!ret)
        {
            sql_err = local_db.lastError();
            sqlerr_str = SQL_LAST_ERR_STR(sql_err);
            DIY_LOG(LOG_LEVEL::LOG_ERROR, "Local db %ls open error!\n%ls",
                    (db_pth + "/" + db_name).utf16(), sqlerr_str.utf16());
            break;
        }

        QList<QString> tbl_name_list;
        if(tbl_name.isEmpty())
        {
            //create all tables.
            tbl_name_list = create_tbl_cmds.keys();
        }
        else
        {
            tbl_name_list << tbl_name;
        }
        foreach (const QString &tbn, tbl_name_list)
        {
            QString cmd = create_tbl_cmds[tbn];
            QSqlQuery query(local_db);
            ret = query.exec(cmd);
            if(!ret)
            {
                sql_err = query.lastError();
                sqlerr_str = SQL_LAST_ERR_STR(sql_err);
                DIY_LOG(LOG_LEVEL::LOG_ERROR,
                        "Create table %ls in %ls fail!\nCmd:%ls\n%ls",
                        tbn.utf16(), local_db.databaseName().utf16(), cmd.utf16(),
                        sqlerr_str.utf16());
                break;
            }
        }

        break;
    }
    return ret;
}

/*
 * This functon may be invoked from multi threads, so it should be thread safe.
*/
bool SkinDatabase::db_ins_err_process(QSqlDatabase &qdb, db_info_intf_t &intf,
                                      QSqlError &cur_sql_err,
                                      QString tbl_name, QString cmd, db_type_t db_type)
{
    QMap<QString, QString> upd_clau_list =
    {
       {m_tbl_expts_str, _FORCE_UPD_TBL_EXPTS_CLAU_(intf, db_type)},
       {m_tbl_objects_str, _FORCE_UPD_TBL_OBJECTS_CLAU_(intf, db_type)},
       {m_tbl_devices_str, _FORCE_UPD_TBL_DEVICES_CLAU_(intf, db_type)},
    };
    QString upd_clau = "";
    QSqlQuery query(qdb);
    QSqlError sql_err;
    QString sqlerr_str;
    quint32 cur_err_code = cur_sql_err.nativeErrorCode().toUInt();
    bool dup = false;
    bool ret = false;

    if(DB_SQLITE == db_type)
    {
        if(cur_err_code == SQLITE_CONSTRAINT_PRIMARYKEY || cur_err_code == SQLITE_CONSTRAINT_UNIQUE)
        {
            dup = true;
        }
    }
    else
    {//assuming mysql
        if(cur_err_code == MYSQL_ER_DUP_ENTRY)
        {
            dup = true;
        }
    }
    if(dup)
    {
        upd_clau = upd_clau_list[tbl_name];
        if(upd_clau.isEmpty())
        {
            DIY_LOG(LOG_LEVEL::LOG_INFO,
                    "Insert but duplicate,cmd:%ls\n"
                    "And, this table has no update clause, so can't insert!",
                    cmd.utf16());
            return false;
        }
        DIY_LOG(LOG_LEVEL::LOG_INFO,
                "Insert but duplicate,cmd:%ls\nNow try updating!", cmd.utf16());
        /*update*/
        cmd.remove(";");
        cmd += " " +  upd_clau + ";";
        ret = query.exec(cmd);
        if(!ret)
        {
            sql_err = query.lastError();
            sqlerr_str = SQL_LAST_ERR_STR(sql_err);
            DIY_LOG(LOG_LEVEL::LOG_ERROR,
                   "Update table %ls in db %ls fail!!!\n"
                   "Cmd:%ls\nsql err info: %ls",
                   tbl_name.utf16(), qdb.databaseName().utf16(),
                   cmd.utf16(), sqlerr_str.utf16());
        }
    }
    else
    {
        sqlerr_str = SQL_LAST_ERR_STR(cur_sql_err);
        DIY_LOG(LOG_LEVEL::LOG_ERROR,
                "Insert table %ls in db %ls fail!!!\n"
                "Cmd:%ls\nsql err info: %ls",
               tbl_name.utf16(), qdb.databaseName().utf16(),
                cmd.utf16(), sqlerr_str.utf16());
    }
    return ret;
}
/*
 * This functon may be invoked from multi threads, so it should be thread safe.
 *
 * Returned:
 *    ret is the last db process result.
 *    safe_ldb_ready: indicates is safe ldb is opened for use.
 *    ret_ind: indicates wether remote db or safe ldb or both are written.
 *    (Note: both remote db and ldb may be written in one invoke of write_db.
 *    e.g. when writting record 1, remote db is availabe, and 1 is written into it;
 *    but when writting record 2, network fails, so 2 is written into safe ldb.)
*/
bool SkinDatabase::write_db(QSqlDatabase &qdb, db_info_intf_t &intf,
                            db_pos_t db_pos, db_type_t db_type, bool use_safe_ldb,
                            QString safe_ldb_pth_str, QString safe_ldb_name_str,
                            QString safe_ldb_conn_name_str,
                            bool *safe_ldb_ready, db_ind_t *ret_ind)
{
    QSqlQuery query(qdb);
    QSqlError sql_err;
    QString sqlerr_str;
    QString name, cmd;
    bool ret = true;
    bool safe_ldb_prepared = safe_ldb_ready ? (*safe_ldb_ready) : false; //only used in case of db_pos being REMOTE

    tv_name_cmd_map_t insert_tbl_cmds[] =
    {
       {m_tbl_expts_str, _INSERT_TBL_EXPTS_CMD_(intf, db_type), intf.expt_changed,
        (REMOTE == db_pos) ? _INSERT_TBL_EXPTS_CMD_(intf, DB_SQLITE) : ""},
       {m_tbl_objects_str, _INSERT_TBL_OBJECTS_CMD_(intf, db_type), intf.obj_changed,
        (REMOTE == db_pos) ? _INSERT_TBL_OBJECTS_CMD_(intf, DB_SQLITE) : ""},
       {m_tbl_devices_str, _INSERT_TBL_DEVICES_CMD_(intf, db_type), intf.dev_changed,
        (REMOTE == db_pos) ? _INSERT_TBL_DEVICES_CMD_(intf, DB_SQLITE) : ""},
       {m_tbl_datum_str, "", true},
    };

    int idx = 0, d_idx = 0;
    while(idx < DIY_SIZE_OF_ARRAY(insert_tbl_cmds))
    {
        name = insert_tbl_cmds[idx].tv_name;
        if(m_tbl_datum_str == name)
        {
            while(d_idx < intf.lambda_data.count())
            {
                cmd =  _INSERT_TBL_DATUM_CMD_(intf, d_idx, db_type);
                ret = query.exec(cmd);
                if(!ret)
                {
                    sql_err = query.lastError();
                    sqlerr_str = SQL_LAST_ERR_STR(sql_err);
                    DIY_LOG(LOG_LEVEL::LOG_ERROR,
                            "Insert table %ls in db %ls fail!!!\n"
                            "Cmd:%ls\nsql err info: %ls",
                           name.utf16(), qdb.databaseName().utf16(),
                            cmd.utf16(),sqlerr_str.utf16());
                    if((REMOTE == db_pos) && use_safe_ldb)
                    {
                        DIY_LOG(LOG_LEVEL::LOG_ERROR, "Insert remote db error."
                                "Now record it in local safe db.");
                        if(!safe_ldb_prepared )
                        {
                            safe_ldb_prepared = prepare_safe_ldb(safe_ldb_pth_str,
                                                         safe_ldb_name_str,
                                                         name, safe_ldb_conn_name_str);
                        }

                        if(safe_ldb_prepared)
                        {
                            cmd = _INSERT_TBL_DATUM_CMD_(intf, d_idx, DB_SQLITE);
                            QSqlDatabase safe_ldb
                                    = QSqlDatabase::database(safe_ldb_conn_name_str);
                            QSqlQuery safe_ldb_q(safe_ldb);
                            ret = safe_ldb_q.exec(cmd);
                            if(!ret)
                            {
                                 sql_err = safe_ldb_q.lastError();
                                 sqlerr_str = SQL_LAST_ERR_STR(sql_err);
                                 DIY_LOG(LOG_LEVEL::LOG_ERROR,
                                        "Insert table %ls in db %ls fail!!!\n"
                                        "Cmd:%ls\nsql err info: %ls",
                                        name.utf16(), safe_ldb.databaseName().utf16(),
                                        cmd.utf16(),sqlerr_str.utf16());
                            }
                            else if(ret_ind)
                            {
                                *ret_ind = (SkinDatabase::db_ind_t)((*ret_ind) | SkinDatabase::DB_SAFE_LDB);
                            }
                        }
                    }
                }
                else if(ret_ind)
                {
                    *ret_ind = (SkinDatabase::db_ind_t)((*ret_ind) | SkinDatabase::DB_REMOTE);
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
                ret = db_ins_err_process(qdb, intf, sql_err, name, cmd, db_type);
                if(!ret && (REMOTE == db_pos) && use_safe_ldb)
                {
                    DIY_LOG(LOG_LEVEL::LOG_ERROR, "Insert remote db error."
                            "Now record it in local safe db.");
                    if(!safe_ldb_prepared )
                    {
                        safe_ldb_prepared = prepare_safe_ldb(safe_ldb_pth_str,
                                                             safe_ldb_name_str,
                                                             name, safe_ldb_conn_name_str);
                    }

                    if(safe_ldb_prepared)
                    {
                        QSqlDatabase safe_ldb
                                = QSqlDatabase::database(safe_ldb_conn_name_str);
                        QSqlQuery safe_ldb_q(safe_ldb);
                        cmd = insert_tbl_cmds[idx].aux_cmd_str;
                        ret = safe_ldb_q.exec(cmd);
                        if(!ret)
                        {
                            sql_err = safe_ldb_q.lastError();
                            ret = db_ins_err_process(safe_ldb, intf, sql_err,
                                                     name, cmd, db_type);
                            if(!ret)
                            {
                                DIY_LOG(LOG_LEVEL::LOG_ERROR,
                                        "Trying to insert safe ldb error!!!");
                            }
                        }
                        else if(ret_ind)
                        {
                            *ret_ind = (SkinDatabase::db_ind_t)((*ret_ind) | SkinDatabase::DB_SAFE_LDB);
                        }
                    }
                }
                else if((REMOTE == db_pos) && ret_ind)
                {
                    *ret_ind = (SkinDatabase::db_ind_t)((*ret_ind) | SkinDatabase::DB_REMOTE);
                }
            }
            else if(ret_ind)
            {
                *ret_ind = (SkinDatabase::db_ind_t)((*ret_ind) | SkinDatabase::DB_REMOTE);
            }
        }
        ++idx;
    }
    if(safe_ldb_ready)
    {
        *safe_ldb_ready = safe_ldb_prepared;
    }
    return ret;
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
            line = _INSERT_VIEW_DATUM_(intf, d_idx, DB_CSV) + "\n";
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
        {
            QSqlDatabase local_db;
            local_db = QSqlDatabase::database(LOCAL_DB_CONN_NAME);
            ret = write_local_db(local_db, m_intf, SkinDatabase::DB_SQLITE);
            local_db.close();
            DIY_LOG(LOG_LEVEL::LOG_INFO, "remove db conn %ls in thread: %u",
                    LOCAL_DB_CONN_NAME.utf16(),
                    (quint64)(QThread::currentThreadId()));
        }
        QSqlDatabase::removeDatabase(LOCAL_DB_CONN_NAME);
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

    if(m_remote_db_info)
    {
        emit write_rdb_sig(m_intf, *m_remote_db_info,
                           m_safe_ldb_dir_str, m_safe_ldb_file_str);
        emit rdb_write_start_sig();
    }

    return ret && ret2;
}

/*invoked from remote worker thread.*/
bool SkinDatabase::write_remote_db(QSqlDatabase &qdb, db_info_intf_t &intf,
                                   db_type_t db_type, bool use_safe_ldb,
                                   QString safe_ldb_pth_str,
                                   QString safe_ldb_name_str,
                                   QString safe_ldb_conn_name_str,
                                   bool &safe_ldb_ready, db_ind_t &ret_ind)
{
    return write_db(qdb, intf, REMOTE, db_type, use_safe_ldb,
                    safe_ldb_pth_str, safe_ldb_name_str,
                    safe_ldb_conn_name_str,&safe_ldb_ready, &ret_ind);
}

void SkinDatabase:: remote_db_write_done_handler(SkinDatabase::db_ind_t write_ind,
                                                 bool ret)
{
    emit rdb_write_done_sig(write_ind, ret);
}

void SkinDatabase::close_dbs(db_ind_t db_ind)
{
    /* This function informs remote-worker thread to close its dbs.
     * Local db is closed in destructor.
    */
    emit close_rdb_sig(db_ind);
}

void SkinDatabase::prepare_remote_db()
{
    emit prepare_rdb_sig(*m_remote_db_info, m_safe_ldb_dir_str, m_safe_ldb_file_str);
}
SkinDatabase::rdb_state_t SkinDatabase::remote_db_st()
{
    return m_rdb_st;
}

void SkinDatabase::rdb_prepare_ret_handler(bool rdb_p)
{
    if(rdb_p)
    {
        m_rdb_st = RDB_ST_OK;
    }
    else
    {
        m_rdb_st = RDB_ST_ERR;
    }
    emit rdb_state_upd_sig(m_rdb_st);
}

void SkinDatabase::upload_safe_ldb(QString safe_ldb_fpn)
{
    emit upload_safe_ldb_sig(safe_ldb_fpn);
}

void SkinDatabase::upload_safe_ldb_done_handler(QList<SkinDatabase::tbl_rec_op_result_t> op_result,
                                                bool result_ret)
{
    emit upload_safe_ldb_end_sig(op_result, result_ret);
}

static void get_sql_select_helper(QMap<QString, void*> &pointer,
                                  SkinDatabase::db_info_intf_t &intf)
{
    pointer.insert(m_tbl_expts_str, &intf.expt_changed);
    pointer.insert(m_tbl_expts_str + "." + m_col_expt_id_str, &intf.expt_id);
    pointer.insert(m_tbl_expts_str + "." + m_col_desc_str, &intf.expt_desc);
    pointer.insert(m_tbl_expts_str + "." + m_col_date_str, &intf.expt_date);
    pointer.insert(m_tbl_expts_str + "." + m_col_time_str, &intf.expt_time);

    pointer.insert(m_tbl_objects_str, &intf.obj_changed);
    pointer.insert(m_tbl_objects_str + "." + m_col_obj_id_str, &intf.obj_id);
    pointer.insert(m_tbl_objects_str + "." + m_col_skin_type_str, &intf.skin_type);
    pointer.insert(m_tbl_objects_str + "." + m_col_desc_str, &intf.obj_desc);

    pointer.insert(m_tbl_devices_str, &intf.dev_changed);
    pointer.insert(m_tbl_devices_str + "." + m_col_dev_id_str, &intf.dev_id);
    pointer.insert(m_tbl_devices_str + "." + m_col_dev_addr_str, &intf.dev_addr);
    pointer.insert(m_tbl_devices_str + "." + m_col_desc_str, &intf.dev_desc);

    pointer.insert(m_tbl_datum_str, nullptr);
    pointer.insert(m_tbl_datum_str + "." + m_col_obj_id_str, &intf.obj_id);
    pointer.insert(m_tbl_datum_str + "." + m_col_pos_str, &intf.pos);
    pointer.insert(m_tbl_datum_str + "." + m_col_lambda_str, &intf.lambda_data);
    pointer.insert(m_tbl_datum_str + "." + m_col_data_str, &intf.lambda_data);
    pointer.insert(m_tbl_datum_str + "." + m_col_date_str, &intf.rec_date);
    pointer.insert(m_tbl_datum_str + "." + m_col_time_str, &intf.rec_time);
    pointer.insert(m_tbl_datum_str + "." + m_col_dev_id_str, &intf.dev_id);
    pointer.insert(m_tbl_datum_str + "." + m_col_expt_id_str, &intf.expt_id);
}

bool SkinDatabase::safe_ldb_to_remote_db(QSqlDatabase &safe_ldb, QSqlDatabase &rdb,
                                         QList<SkinDatabase::tbl_rec_op_result_t>& op_result)
{
    typedef struct
    {
        QString tbl_name;
        QString tbl_cols;
        QString primary_id_str;
    }tbl_name_cols_map_t;
    tbl_name_cols_map_t tbl_name_cols_map[] =
    {
        {m_tbl_expts_str, _TBL_EXPTS_COLS_, m_col_expt_id_str},
        {m_tbl_objects_str, _TBL_OBJECTS_COLS_, m_col_obj_id_str},
        {m_tbl_devices_str, _TBL_DEVICES_COLS_, m_col_dev_id_str},
        {m_tbl_datum_str, _TBL_DATUM_COLS_ + m_col_rec_id_str, m_col_rec_id_str},
    };
    QMap<QString, void*> helper;
    db_info_intf_t intf;
    QSqlQuery safe_ldb_q(safe_ldb);
    bool ret = true, result_ret = true;
    db_lambda_data_s_t lambda_data_p;
    bool one_data_item = false;
    SkinDatabase::tbl_rec_op_result_t tbl_op_ret;

    get_sql_select_helper(helper, intf);
    for(int idx = 0; idx < DIY_SIZE_OF_ARRAY(tbl_name_cols_map); idx++)
    {
        QString tbl_name = tbl_name_cols_map[idx].tbl_name;
        QString cols = tbl_name_cols_map[idx].tbl_cols;
        QStringList col_list = cols.split(",");
        QString primary_key_str = tbl_name_cols_map[idx].primary_id_str;
        QString primary_key_val;

        tbl_op_ret.tbl_name = tbl_name;
        tbl_op_ret.rec_succ = tbl_op_ret.rec_fail = tbl_op_ret.rec_part_succ = 0;
        QString cmd = QString("SELECT ") + cols + " FROM " + tbl_name + ";";
        safe_ldb_q.exec(cmd);
        while(safe_ldb_q.next())
        {
            intf.lambda_data.clear();
            intf.dev_changed = intf.obj_changed = intf.expt_changed = false;
            if(m_tbl_datum_str == tbl_name)
            {
                one_data_item = false;
                for(int c_idx = 0; c_idx < col_list.count(); c_idx++)
                {
                    if((m_col_lambda_str == col_list[c_idx])
                            || (m_col_data_str == col_list[c_idx]))
                    {
                        if(m_col_lambda_str == col_list[c_idx])
                        {
                            lambda_data_p.lambda = safe_ldb_q.value(c_idx).toUInt();
                        }
                        else
                        {
                            lambda_data_p.data = safe_ldb_q.value(c_idx).toULongLong();
                        }
                        if(!one_data_item)
                        {
                            one_data_item = true;
                        }
                        else
                        {
                            (*(QList<db_lambda_data_s_t>*)(helper[tbl_name + "." + col_list[c_idx]])).append(lambda_data_p);
                        }
                    }
                    else if(m_col_rec_id_str != col_list[c_idx])
                    {
                        *(QString*)(helper[tbl_name + "." + col_list[c_idx]])
                                = safe_ldb_q.value(c_idx).toString();
                        if(col_list[c_idx] == primary_key_str)
                        {
                            primary_key_val = safe_ldb_q.value(c_idx).toString();
                        }
                    }
                }
            }
            else
            {
                for(int c_idx = 0; c_idx < col_list.count(); c_idx++)
                {
                    *(QString*)(helper[tbl_name + "." + col_list[c_idx]])
                            = safe_ldb_q.value(c_idx).toString();
                    if(col_list[c_idx] == primary_key_str)
                    {
                        primary_key_val = safe_ldb_q.value(c_idx).toString();
                    }
                }
                *(bool*)(helper[tbl_name]) = true;
            }
            /*now write it into rdb.*/
            ret = SkinDatabase::write_db(rdb, intf, SkinDatabase::REMOTE,
                                         SkinDatabase::DB_MYSQL);
            if(ret)
            {
                /*Delete this record in safe ldb.*/
                QString cmd = QString("DELETE FROM %1 WHERE %2=\"%3\";").arg(tbl_name,
                                                                             primary_key_str,
                                                                             primary_key_val);
                QSqlQuery del_safe_ldb_q(safe_ldb);
                ret = del_safe_ldb_q.exec();
                if(!ret)
                {
                    ++(tbl_op_ret.rec_part_succ);
                    DIY_LOG(LOG_LEVEL::LOG_WARN,
                            "Record has been written into remote db,"
                            "but the following command to delete it from safe ldb fails:\n%ls\n"
                            "If the safe ldb file is not deleted automatically, please delete it manually to avoid redundant records in remote db when you perform other upload using this safe ldb file.",
                            cmd.utf16());
                }
                else
                {
                    ++(tbl_op_ret.rec_succ);
                }
            }
            else
            {
                ++(tbl_op_ret.rec_fail);
                DIY_LOG(LOG_LEVEL::LOG_ERROR, "Upload safe ldb record error!!!");
            }
        }
        tbl_op_ret.rec_cnt = tbl_op_ret.rec_fail + tbl_op_ret.rec_part_succ
                            + tbl_op_ret.rec_succ;
        op_result.append(tbl_op_ret);
        result_ret = result_ret && (tbl_op_ret.rec_fail == 0);
    }
    return result_ret;
}
////////////////////////////////////////////////////////////////
void remove_qt_sqldb_conn(QString conn_name)
{
    {
        QSqlDatabase local_db;
        local_db = QSqlDatabase::database(conn_name);
        local_db.close();
    }
    QSqlDatabase::removeDatabase(conn_name);
    DIY_LOG(LOG_LEVEL::LOG_INFO, "remove db conn %ls in thread: %u",
            conn_name.utf16(),
            (quint64)(QThread::currentThreadId()));
}
