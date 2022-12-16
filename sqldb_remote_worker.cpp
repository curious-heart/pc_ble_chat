#include <QSqlError>
#include "logger.h"
#include "sqldb_remote_worker.h"

#define REMOTE_DB_CONN_NAME QString("remote_mysql_conntection")

SqlDbRemoteWorker::SqlDbRemoteWorker()
{}

SqlDbRemoteWorker::~SqlDbRemoteWorker()
{
    if(m_remote_db_ready)
    {
        remove_qt_sqldb_conn(REMOTE_DB_CONN_NAME);
        m_remote_db_ready = false;
    }
}

bool SqlDbRemoteWorker::prepare_rdb(setting_db_info_t db_info)
{
    if(!m_remote_db_ready)
    {
        while(true)
        {
            QSqlDatabase remote_db;

            remote_db = QSqlDatabase::addDatabase(QString("QMYSQL"), REMOTE_DB_CONN_NAME);
            if(!remote_db.isValid())
            {
                QSqlError sqlerr;
                sqlerr = remote_db.lastError();
                DIY_LOG(LOG_LEVEL::LOG_ERROR,
                        "addDatabase QMYSQL error!\n"
                        "errtxt:%ls\nnative_errcode:%ls\nerrtype:%d",
                        sqlerr.text().utf16(), sqlerr.nativeErrorCode().utf16(),
                        (int)sqlerr.type());
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
                QString err = db_info.log_print(false);
                DIY_LOG(LOG_LEVEL::LOG_ERROR,
                        "Open remote db error!\nThe db info is as the following:%ls\n",
                        err.utf16());
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
            emit remote_db_prepared();
            break;
        }
        if(!m_remote_db_ready)
        {
            QSqlDatabase::removeDatabase(REMOTE_DB_CONN_NAME);
        }
    }
    return m_remote_db_ready;
}

bool SqlDbRemoteWorker::write_rdb(SkinDatabase::db_info_intf_t intf)
{
    bool ret = false;
    if(m_remote_db_ready)
    {
        QSqlDatabase remote_db;

        remote_db = QSqlDatabase::database(REMOTE_DB_CONN_NAME);
        ret = SkinDatabase::write_remote_db(remote_db, intf,
                                            SkinDatabase::DB_MYSQL);
        if(!ret)
        {
            emit remote_db_write_error();
            DIY_LOG(LOG_LEVEL::LOG_ERROR, "Write remote error!");
        }
        else
        {
            emit remote_db_write_done();
        }
    }
    else
    {
        DIY_LOG(LOG_LEVEL::LOG_ERROR, "Remote db not prepared!");
    }
    return ret;
}

bool SqlDbRemoteWorker::close_rdb()
{
    if(m_remote_db_ready)
    {
        remove_qt_sqldb_conn(REMOTE_DB_CONN_NAME);
        m_remote_db_ready = false;
        emit remote_db_closed();
    }
    return true;
}
