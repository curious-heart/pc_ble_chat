#include <QSqlQuery>
#include "sqldb_works.h"
#include "logger.h"

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

void SkinDatabase::store_these_info(db_info_intf_t &info)
{
    m_intf = info;
    if(!prepare_local_db())
    {}
}

bool SkinDatabase::create_tbls_and_views()
{
    QSqlQuery query;
    bool ret = true;
    QMap<QString, col_def_func_t>::iterator it;
    col_def_func_t func;

    it = m_tbl_create_fac.begin();
    while(it != m_tbl_create_fac.end())
    {
        QString cmd;
        func = it.value();
        cmd = "CREATE TABLE IF NOT EXISTS " + it.key()
                + " (" + (this->*func)() + ");";
        query = m_local_db.exec(cmd);
        ret = query.exec();
        if(!ret)
        {
           DIY_LOG(LOG_LEVEL::LOG_ERROR, "Create table %ls fail!", it.key().utf16());
           return ret;
        }
        ++it;
    }

    it = m_view_create_fac.begin();
    while(it != m_view_create_fac.end())
    {
        QString cmd;
        func = it.value();
        cmd = "CREATE VIEW IF NOT EXISTS " + it.key()
                + " (" + (this->*func)() + ");";
        query = m_local_db.exec(cmd);
        ret = query.exec();
        if(!ret)
        {
           DIY_LOG(LOG_LEVEL::LOG_ERROR, "Create view %ls fail!", it.key().utf16());
           return ret;
        }
        ++it;
    }

    return ret;
}

QString SkinDatabase::create_expt_tbl_cmd()
{
    QString cmd;
    cmd = "CREATE TABLE IF NOT EXISTS " + m_tbl_expts_str
            + " (" + expt_tbl_col_def() + ");";

    return cmd;
}

QString SkinDatabase::create_objects_tbl_cmd()
{
    QString cmd;
    cmd = "CREATE TABLE IF NOT EXISTS " + m_tbl_objects_str
            + " (" + expt_tbl_col_def() + ");";

    return cmd;
}

QString SkinDatabase::create_devices_tbl_cmd()
{
    QString cmd;
    cmd = "CREATE TABLE IF NOT EXISTS " + m_tbl_devices_str
            + " (" + expt_tbl_col_def() + ");";

    return cmd;
}
