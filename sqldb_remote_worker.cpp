#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include "logger.h"
#include "sqldb_remote_worker.h"

#define REMOTE_DB_CONN_NAME QString("remote_mysql_conntection")
#define SAFE_LOCAL_DB_CONN_NAME QString("safe_local_db_connection")
#define UPLOAD_SAFE_LDB_CONN_NAME QString("upload_safe_ldb_connection")

SqlDbRemoteWorker::SqlDbRemoteWorker()
{
    DIY_LOG(LOG_LEVEL::LOG_INFO, "+++++++++SqlDbRemoteWorker constructor in thread: %u",
            (quint64)(QThread::currentThreadId()));
}

SqlDbRemoteWorker::~SqlDbRemoteWorker()
{
    DIY_LOG(LOG_LEVEL::LOG_INFO, "----------SqlDbRemoteWorker destructor in thread: %u",
            (quint64)(QThread::currentThreadId()));
    if(m_remote_db_ready)
    {
        remove_qt_sqldb_conn(REMOTE_DB_CONN_NAME);
        m_remote_db_ready = false;
    }
    if(m_safe_ldb_ready)
    {
        remove_qt_sqldb_conn(SAFE_LOCAL_DB_CONN_NAME);
        m_safe_ldb_ready = false;
    }
}

bool SqlDbRemoteWorker::prepare_rdb(setting_rdb_info_t db_info, QString safe_ldb_dir_str,
                                    QString safe_ldb_file_str)
{
    m_safe_ldb_dir_str = safe_ldb_dir_str;
    m_safe_ldb_file_str =  safe_ldb_file_str;
    DIY_LOG(LOG_LEVEL::LOG_INFO, "prepare_rdb...");
    if(!m_remote_db_ready)
    {
        DIY_LOG(LOG_LEVEL::LOG_INFO, "prepare_rdb...not ready, begin to prepare.");

        QSqlError sql_err;
        QString sqlerr_str;

        while(true)
        {
            QSqlDatabase remote_db;

            DIY_LOG(LOG_LEVEL::LOG_INFO, "prepare rdb: add database.");
            remote_db = QSqlDatabase::addDatabase(QString("QMYSQL"), REMOTE_DB_CONN_NAME);
            if(!remote_db.isValid())
            {
                sql_err = remote_db.lastError();
                sqlerr_str = SQL_LAST_ERR_STR(sql_err);
                DIY_LOG(LOG_LEVEL::LOG_ERROR,
                        "addDatabase QMYSQL error!\nsql err info:%ls",
                        sqlerr_str.utf16());
                m_remote_db_ready = false;
                break;
            }
            remote_db.setHostName(db_info.srvr_addr);
            remote_db.setPort(db_info.srvr_port);
            remote_db.setUserName(db_info.login_id);
            remote_db.setPassword(db_info.login_pwd);
            remote_db.setDatabaseName(db_info.db_name);
            DIY_LOG(LOG_LEVEL::LOG_INFO, "prepare rdb: open database.");
            m_remote_db_ready = remote_db.open();
            if(!m_remote_db_ready)
            {
                sql_err = remote_db.lastError();
                sqlerr_str = SQL_LAST_ERR_STR(sql_err);
                QString err = db_info.log_print(false);
                DIY_LOG(LOG_LEVEL::LOG_ERROR,
                        "Open remote db error! The db info is as the following:\n%ls\n"
                        "sql error info:%ls",
                        err.utf16(), sqlerr_str.utf16());
                break;
            }

            DIY_LOG(LOG_LEVEL::LOG_INFO, "prepare rdb: create tbls and views.");
            m_remote_db_ready
                    = SkinDatabase::create_tbls_and_views(remote_db,
                                                          SkinDatabase::REMOTE,
                                                          SkinDatabase::DB_MYSQL);
            if(!m_remote_db_ready)
            {
                remote_db.close();
                QString err = db_info.log_print(false);
                DIY_LOG(LOG_LEVEL::LOG_ERROR,
                        "Prepare remote db error!\nThe db info is as the following:%ls\n",
                        err.utf16());
                break;
            }
            break;
        }
        if(!m_remote_db_ready)
        {
            QSqlDatabase::removeDatabase(REMOTE_DB_CONN_NAME);
        }
    }

    emit remote_db_prepared_sig(m_remote_db_ready);
    if(m_remote_db_ready)
    {
        DIY_LOG(LOG_LEVEL::LOG_INFO, "Remote db prepared.");
    }
    else
    {
        DIY_LOG(LOG_LEVEL::LOG_ERROR, "Remote db NOT prepared!");
    }
    return m_remote_db_ready;
}

bool SqlDbRemoteWorker::write_rdb(SkinDatabase::db_info_intf_t intf,
                                  setting_rdb_info_t rdb_info,
                                  QString safe_ldb_dir_str, QString safe_ldb_file_str)
{
    bool ret = false;
    SkinDatabase::db_ind_t ret_ind = SkinDatabase::DB_NONE;

    prepare_rdb(rdb_info, safe_ldb_dir_str, safe_ldb_file_str);
    if(m_remote_db_ready)
    {
        DIY_LOG(LOG_LEVEL::LOG_INFO, "Begin to write remote db...");
        QSqlDatabase remote_db;

        remote_db = QSqlDatabase::database(REMOTE_DB_CONN_NAME);
        ret = SkinDatabase::write_remote_db(remote_db, intf,
                                            SkinDatabase::DB_MYSQL, true,
                                            m_safe_ldb_dir_str, m_safe_ldb_file_str,
                                            SAFE_LOCAL_DB_CONN_NAME,
                                            m_safe_ldb_ready, ret_ind);
        if(!ret)
        {
            DIY_LOG(LOG_LEVEL::LOG_ERROR, "Write remote db error!");
        }
        else
        {
            DIY_LOG(LOG_LEVEL::LOG_INFO, "Write remote db finished.");
        }
    }
    else
    {
        DIY_LOG(LOG_LEVEL::LOG_WARN, "Since remote db is not prepared, write safe local db.");
        ret = SkinDatabase::prepare_safe_ldb(m_safe_ldb_dir_str, m_safe_ldb_file_str, "",
                                             SAFE_LOCAL_DB_CONN_NAME);
        m_safe_ldb_ready = ret;
        if(!ret)
        {
            DIY_LOG(LOG_LEVEL::LOG_ERROR,
                    "Prepare safe ldb fail, data can't be written into it!");
        }
        else
        {
            QSqlDatabase safe_ldb;
            safe_ldb = QSqlDatabase::database(SAFE_LOCAL_DB_CONN_NAME);
            ret = SkinDatabase::write_local_db(safe_ldb, intf, SkinDatabase::DB_SQLITE);
        }
        ret_ind = SkinDatabase::DB_SAFE_LDB;
    }
    DIY_LOG(LOG_LEVEL::LOG_INFO, "remote_db_write_done_sig");
    emit remote_db_write_done_sig(ret_ind, ret);
    return ret;
}

bool SqlDbRemoteWorker::close_rdb(SkinDatabase::db_ind_t db_ind)
{
    if(db_ind & SkinDatabase::DB_REMOTE)
    {
        if(m_remote_db_ready)
        {
            remove_qt_sqldb_conn(REMOTE_DB_CONN_NAME);
            m_remote_db_ready = false;
            emit remote_db_closed_sig();
        }
        DIY_LOG(LOG_LEVEL::LOG_INFO, ".............. remote db is closed in thread: %u",
                (quint64)(QThread::currentThreadId()));
    }

    if(db_ind & SkinDatabase::DB_SAFE_LDB)
    {
        remove_qt_sqldb_conn(SAFE_LOCAL_DB_CONN_NAME);
        m_safe_ldb_ready = false;
    }

    return true;
}

bool SqlDbRemoteWorker::upload_safe_ldb_to_rdb(QString safe_ldb_fpn)
{
    bool ret = true;
    QList<SkinDatabase::tbl_rec_op_result_t> op_result;

    DIY_LOG(LOG_LEVEL::LOG_INFO, "upload_safe_ldb_to_rdb");
    op_result.clear();
    while(true)
    {
        QSqlDatabase safe_ldb, rdb;

        DIY_LOG(LOG_LEVEL::LOG_INFO, "safe ldb: add database");
        safe_ldb = QSqlDatabase::addDatabase(QString("QSQLITE"),
                                             UPLOAD_SAFE_LDB_CONN_NAME);
        if(!safe_ldb.isValid())
        {
            DIY_LOG(LOG_LEVEL::LOG_ERROR, "Add database safe ldb %ls error!!!",
                    UPLOAD_SAFE_LDB_CONN_NAME.utf16());
            ret = false;
            break;
        }
        safe_ldb.setDatabaseName(safe_ldb_fpn);
        DIY_LOG(LOG_LEVEL::LOG_INFO, "safe ldb: open database");
        safe_ldb.open();
        if(!safe_ldb.isOpen())
        {
            DIY_LOG(LOG_LEVEL::LOG_ERROR, "Open database safe ldb %ls error!!!",
                    safe_ldb_fpn.utf16());
            ret = false;
            break;
        }

        /*Remote db should have been prepared before reach here.*/
        if(!m_remote_db_ready)
        {
            DIY_LOG(LOG_LEVEL::LOG_ERROR, "Remote not prepared, can't upload safe ldb!");
            safe_ldb.close();
            ret = false;
            break;
        }
        rdb = QSqlDatabase::database(REMOTE_DB_CONN_NAME);

        DIY_LOG(LOG_LEVEL::LOG_INFO, "safe ldb: safe_ldb_to_remote_db");
        ret = SkinDatabase::safe_ldb_to_remote_db(safe_ldb, rdb, op_result);
        safe_ldb.close();

        break;
    }

    /* We just close and remove safe ldb conntction here, but remain remote db connection,
     * because there maybe other safe ldbs for uploading.
    */
    QSqlDatabase::removeDatabase(UPLOAD_SAFE_LDB_CONN_NAME);
    if(ret)
    {
        DIY_LOG(LOG_LEVEL::LOG_INFO, "All recoreds in safe ldb has been upload to remote db, now delete the safe ldb file.");
        QFile sldbf(safe_ldb_fpn);
        ret = sldbf.remove();
        if(!ret)
        {
            DIY_LOG(LOG_LEVEL::LOG_WARN, "Delete safe ldb file error! PLease delete it manually later.");
        }
    }
    DIY_LOG(LOG_LEVEL::LOG_INFO, "safe ldb: upload_safe_ldb_done_sig");
    emit upload_safe_ldb_done_sig(op_result, ret);
    return ret;
}
