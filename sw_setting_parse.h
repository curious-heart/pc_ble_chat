#ifndef SW_SETTING_PARSE_H
#define SW_SETTING_PARSE_H

#include <QString>
#include <QList>
#include <QFile>
#include <QDebug>

typedef class _ble_dev_info_t
{
public:
    QString addr;
    QString srv_uuid;
    QString rx_char_uuid, tx_char_uuid;

    _ble_dev_info_t()
    {
        clear();
    }
    void clear()
    {
        addr = srv_uuid = rx_char_uuid = tx_char_uuid = QString();
    }
    void log_print()
    {
       qDebug() << "dev addr: " << addr
                << "dev srv_uuid: " << srv_uuid
                << "dev rx_char_uuid: " << rx_char_uuid
                << "dev tx_char_uuid: " << tx_char_uuid;
    }
}setting_ble_dev_info_t;

typedef class _db_info_t
{
public:
    bool valid;
    QString srvr_addr;
    uint16_t srvr_port;
    QString db_name, login_id, login_pwd;
    QString dbms_name, dbms_ver;

    _db_info_t()
    {
        clear();
    }
    void clear()
    {
        srvr_addr = db_name = login_id = login_pwd = dbms_name = dbms_ver = QString();
        srvr_port = 0;
        valid = false;
    }
    void log_print()
    {
        qDebug() << "db_info srvr_addr" << srvr_addr
                 << "db_info srvr_port" << srvr_port
                 << "db_info db_name" << db_name
                 << "db_info login_id" << login_id
                 << "db_info login_pwd" << login_pwd
                 << "db_info dbms_name" << dbms_name
                 << "db_info dbms_ver" << dbms_ver;
    }
}setting_db_info_t;

typedef class _oth_settings_t
{
public:
    bool use_remote_db;
    _oth_settings_t()
    {
        clear();
    }
    void clear()
    {
        use_remote_db = true;
    }
    void log_print()
    {
        qDebug() << "oth_settings use_remote_db: " << use_remote_db;
    }
}setting_oth_t;

typedef class _sw_settings_t
{
public:
    //QList<setting_ble_dev_info_t*> ble_dev_list;
    QMap<QString, setting_ble_dev_info_t*> ble_dev_list;
    setting_db_info_t db_info;
    setting_oth_t oth_settings;

    _sw_settings_t()
    {
        ble_dev_list.clear();
        db_info.clear();
        oth_settings.clear();
    }
    ~_sw_settings_t()
    {
        QMap<QString, setting_ble_dev_info_t*>::iterator it
                                                = ble_dev_list.begin();
        while(it != ble_dev_list.end())
        {
            delete it.value();
            ++it;
        }
        //qDeleteAll(ble_dev_list.values().begin(), ble_dev_list.end());
        ble_dev_list.clear();
    }
}sw_settings_t;

bool load_sw_settings_from_xml(sw_settings_t &loaded);
void clear_loaded_settings(sw_settings_t &loaded);
#endif // SW_SETTING_PARSE_H
