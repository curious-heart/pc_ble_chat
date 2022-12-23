#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include "logger.h"
#include "sqldb_remote_worker.h"

#define REMOTE_DB_CONN_NAME QString("remote_mysql_conntection")
#define SAFE_LOCAL_DB_CONN_NAME QString("safe_local_db_connection")

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
}

bool SqlDbRemoteWorker::prepare_rdb(setting_rdb_info_t db_info, QString safe_ldb_dir_str,
                                    QString safe_ldb_file_str)
{
    if(!m_remote_db_ready)
    {
        QSqlError sql_err;
        QString sqlerr_str;

        m_safe_ldb_dir_str = safe_ldb_dir_str;
        m_safe_ldb_file_str =  safe_ldb_file_str;
        while(true)
        {
            QSqlDatabase remote_db;

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

    prepare_rdb(rdb_info, safe_ldb_dir_str, safe_ldb_file_str);
    if(m_remote_db_ready)
    {
        QSqlDatabase remote_db;
        SkinDatabase::db_ind_t ret_ind = SkinDatabase::DB_NONE;

        remote_db = QSqlDatabase::database(REMOTE_DB_CONN_NAME);
        ret = SkinDatabase::write_remote_db(remote_db, intf, SkinDatabase::DB_MYSQL,
                                            m_safe_ldb_dir_str, m_safe_ldb_file_str,
                                            SAFE_LOCAL_DB_CONN_NAME,
                                            m_safe_ldb_ready, ret_ind);
        if(!ret)
        {
            emit remote_db_write_error_sig();
            DIY_LOG(LOG_LEVEL::LOG_ERROR, "Write remote error!");
        }
        else
        {
            emit remote_db_write_done_sig();
        }
    }
    else
    {
        DIY_LOG(LOG_LEVEL::LOG_WARN, "Since remote db is not prepared, write safe local db.");
        QSqlDatabase safe_ldb;
        bool ret;
        ret = SkinDatabase::prepare_safe_ldb(m_safe_ldb_dir_str, m_safe_ldb_file_str, "",
                                             SAFE_LOCAL_DB_CONN_NAME);
        if(!ret)
        {
            DIY_LOG(LOG_LEVEL::LOG_ERROR,
                    "Prepare safe ldb fail, data can't be written into it!");
        }
        else
        {
            safe_ldb = QSqlDatabase::database(SAFE_LOCAL_DB_CONN_NAME);
            ret = SkinDatabase::write_local_db(safe_ldb, intf, SkinDatabase::DB_SQLITE);
        }
    }
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
    }

    return true;
}

bool SqlDbRemoteWorker::upload_safe_ldb_to_rdb(QString safe_ldb_fpn)
{
    emit upload_safe_ldb_done_sig();
    return true;
}
