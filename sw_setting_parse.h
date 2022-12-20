#ifndef SW_SETTING_PARSE_H
#define SW_SETTING_PARSE_H

#include <QString>
#include <QList>
#include <QMap>
#include <QFile>
#include <QDebug>
#include "diy_common_tool.h"

typedef struct _light_info_t
{
    quint32 lambda; /*波长(nm)*/
    int flash_period; /*点亮时长(ms)*/
    int flash_gap; /*熄灭后到下一个灯点亮前的间隔时间(ms)*/
    int idx; /*设备内部对灯的编号。从1开始*/
}light_info_t;
typedef QMap<int, light_info_t*> light_list_t;

typedef class _ble_dev_info_t
{
public:
    QString addr;
    QString srv_uuid;
    QString rx_char_uuid, tx_char_uuid;
    QString dev_type;
    light_list_t light_list;
    int single_light_wait_time;

    _ble_dev_info_t()
    {
        clear_all();
    }
    ~_ble_dev_info_t()
    {
        clear_all();
    }
    void clear_light_list()
    {
        light_list_t::iterator it = light_list.begin();
        while(it != light_list.end())
        {
            delete it.value();
            ++it;
        }
        light_list.clear();
    }
    void clear_all()
    {
        addr = srv_uuid = rx_char_uuid = tx_char_uuid = QString();
        dev_type = QString();
        clear_light_list();
        single_light_wait_time = 0;
    }
    void log_print()
    {
       qDebug() << "dev addr: " << addr << ","
                << "dev dev_type: " << dev_type << ","
                << "dev srv_uuid: " << srv_uuid << ","
                << "dev rx_char_uuid: " << rx_char_uuid << ","
                << "dev tx_char_uuid: " << tx_char_uuid << "\n";
       qDebug() << "dev light_list: " << "\n";
       light_list_t::iterator it = light_list.begin();
       while(it != light_list.end())
       {
           light_info_t* info = it.value();
           qDebug() << "lambda:" << info->lambda << ","
                    << "flash_period:" << info->flash_period << ","
                    << "flash_gap" << info->flash_gap << "\n";
       }
       qDebug() << "\n";
    }
}setting_ble_dev_info_t;

typedef class _oth_settings_t
{
public:
    bool use_remote_db;
    _oth_settings_t()
    {
        clear();
    }
    ~_oth_settings_t()
    {
        clear();
    }
    void clear()
    {
        use_remote_db = true;
    }
    void log_print()
    {
        qDebug() << "oth_settings use_remote_db: " << use_remote_db << "\n";
    }
}setting_oth_t;

typedef class _sw_settings_t
{
public:
    QMap<QString, setting_ble_dev_info_t*> ble_dev_list;
    setting_rdb_info_t db_info;
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
        ble_dev_list.clear();
    }
}sw_settings_t;

typedef QMap<QString, bool> str_bool_map_t;
void load_sw_settings(sw_settings_t &sw_s);
void clear_loaded_settings(sw_settings_t &loaded);
#endif // SW_SETTING_PARSE_H
