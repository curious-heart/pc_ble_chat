#include <QtXml>
#include <QFile>
#include <QMap>

#include "sw_setting_parse.h"
#include "types_and_defs.h"
#include "diy_common_tool.h"

static inline QString ble_dev_list_elem() { return QStringLiteral("ble_dev_list");}
static inline QString dev_elem() { return QStringLiteral("dev");}
static inline QString addr_elem() { return QStringLiteral("addr");}
static inline QString srv_uuid_elem() { return QStringLiteral("srv_uuid");}
static inline QString rx_char_elem() { return QStringLiteral("rx_char");}
static inline QString tx_char_elem() { return QStringLiteral("tx_char");}
static inline QString db_info_elem() { return QStringLiteral("db_info");}
static inline QString srvr_addr_elem() { return QStringLiteral("srvr_addr");}
static inline QString srvr_port_elem() { return QStringLiteral("srvr_port");}
static inline QString dbms_name_elem() { return QStringLiteral("dbms_name");}
static inline QString dbms_ver_elem() { return QStringLiteral("dbms_ver");}
static inline QString db_name_elem() { return QStringLiteral("db_name");}
static inline QString login_id_elem() { return QStringLiteral("login_id");}
static inline QString login_pwd_elem() { return QStringLiteral("login_pwd");}
static inline QString oth_settings_elem() { return QStringLiteral("oth_settings");}
static inline QString use_remote_db_elem() { return QStringLiteral("use_remote_db");}

typedef bool (*parser_t)(QDomElement &n, sw_settings_t &loaded);
static QMap<QString, parser_t> xml_parser;

static bool check_setting_device_info(setting_ble_dev_info_t * dev_info)
{
    if(nullptr == dev_info)
    {
        qDebug() << "dev_info is a nullptr";
        return false;
    }

    bool ret = true;
    if(!isMacAddress(dev_info->addr)
        || !isFullUUID(dev_info->srv_uuid)
        || !isFullUUID(dev_info->rx_char_uuid)
        || !isFullUUID(dev_info->tx_char_uuid))
    {
        qDebug() << "There is some error in setting_device_info: ";
        ret = false;
    }
    dev_info->log_print();
    return ret;
}

static bool check_setting_db_info(setting_db_info_t* db_info)
{
    if(nullptr == db_info)
    {
        qDebug() << "db_info is a nullptr";
        return false;
    }
    bool ret = true;
    if(db_info->srvr_addr.isEmpty()|| (0 == db_info->srvr_port)
            || db_info->db_name.isEmpty())
    {
        qDebug() << "There is some error in setting_db_info: ";
        ret = false;
    }
    db_info->log_print();
    return ret;
}

static bool parse_dev_list(QDomElement &e, sw_settings_t &loaded)
{
    QDomElement dev_e = e.firstChildElement();
    bool dev_empty_flag = true;
    while(!dev_e.isNull())
    {
        if(dev_e.tagName() == dev_elem())
        {
            setting_ble_dev_info_t * dev_info = new setting_ble_dev_info_t;
            if(!dev_info)
            {
                qDebug() << "new setting_ble_dev_info_t fail!";
                return false;
            }
            QDomElement info_e = dev_e.firstChildElement();
            while(!info_e.isNull())
            {
                if(info_e.tagName() == addr_elem())
                {
                    dev_info->addr = info_e.text();
                }
                else if(info_e.tagName() == srv_uuid_elem())
                {
                    dev_info->srv_uuid = info_e.text();
                }
                else if(info_e.tagName() == rx_char_elem())
                {
                    dev_info->rx_char_uuid = info_e.text();
                }
                else if(info_e.tagName() == tx_char_elem())
                {
                    dev_info->tx_char_uuid = info_e.text();
                }
                else
                {
                    qDebug() << "Unknown element " << info_e.tagName() << " found, not processed";
                }
                info_e = info_e.nextSiblingElement();
            }
            //validation of dev_info
            if(check_setting_device_info(dev_info))
            {
                //loaded.ble_dev_list.append(dev_info);
                loaded.ble_dev_list.insert(dev_info->addr, dev_info);
                dev_empty_flag = false;
            }
            else
            {
                qDebug() << "invalid device information";
            }
        }
        else
        {
            qDebug() << "Unknown element " << dev_e.tagName() << " found, not processed";
        }
        dev_e = dev_e.nextSiblingElement();
    }
    if(dev_empty_flag)
    {
        qDebug() << "No valid device infomation element found!";
        return false;
    }
    else
    {
        return true;
    }
}

static bool parse_db_info(QDomElement &e, sw_settings_t &loaded)
{
    QDomElement info_e = e.firstChildElement();

    while(!info_e.isNull())
    {
        if(info_e.tagName() == srvr_addr_elem())
        {
            loaded.db_info.srvr_addr = info_e.text();
        }
        else if(info_e.tagName() == srvr_port_elem())
        {
            loaded.db_info.srvr_port = info_e.text().toUShort();
        }
        else if(info_e.tagName() == db_name_elem())
        {
            loaded.db_info.db_name = info_e.text();
        }
        else if(info_e.tagName() == login_id_elem())
        {
            loaded.db_info.login_id = info_e.text();
        }
        else if(info_e.tagName() == login_pwd_elem())
        {
            loaded.db_info.login_pwd = info_e.text();
        }
        else if(info_e.tagName() == dbms_name_elem())
        {
            loaded.db_info.dbms_name = info_e.text();
        }
        else if(info_e.tagName() == dbms_ver_elem())
        {
            loaded.db_info.dbms_ver = info_e.text();
        }
        else
        {
            qDebug() << "Unknown tag in " << db_info_elem() << " :" << info_e.tagName();
        }
        info_e = info_e.nextSiblingElement();
    }

    loaded.db_info.valid = check_setting_db_info(&loaded.db_info);
    return true;
}

static bool parse_oth_settings(QDomElement &e, sw_settings_t &loaded)
{
    QDomElement info_e = e.firstChildElement();
    while(!info_e.isNull())
    {
        if(info_e.tagName() == use_remote_db_elem())
        {
            loaded.oth_settings.use_remote_db = (bool)(info_e.text().toInt());
        }
        else
        {
            qDebug() << "Unknown tag in " << oth_settings_elem()
                     << " :" << info_e.tagName();
        }
        info_e = info_e.nextSiblingElement();
    }
    return true;
}

static void init_sw_settings_xml_parser()
{
    xml_parser.insert(ble_dev_list_elem(), parse_dev_list);
    xml_parser.insert(db_info_elem(), parse_db_info);
    xml_parser.insert(oth_settings_elem(), parse_oth_settings);
}

bool load_sw_settings_from_xml(sw_settings_t &loaded)
{
    QFile xml_file(g_settings_xml_fpn);
    QDomDocument doc;

    if(!xml_file.open(QIODevice::ReadOnly))
    {
        qDebug() << "Open XML file " << g_settings_xml_fpn << " fail!";
        return false;
    }
    if(!doc.setContent(&xml_file))
    {
        qDebug() << "DomDocument setContent fail";
        return false;
    }
    xml_file.close();

    init_sw_settings_xml_parser();

    QDomElement docElem = doc.documentElement();
    QDomElement c_e = docElem.firstChildElement();
    bool parse_error = false;
    while(!c_e.isNull())
    {
        QString key = c_e.tagName();
        parser_t parser = xml_parser.value(key, nullptr);
        if(!parser || !parser(c_e, loaded))
        {
            parse_error = true;
            break;
        }
        c_e = c_e.nextSiblingElement();
    }
    if(parse_error)
    {
        clear_loaded_settings(loaded);
        return false;
    }
    return true;
}

void clear_loaded_settings(sw_settings_t &loaded)
{
    QMap<QString, setting_ble_dev_info_t*>::iterator it
                                            = loaded.ble_dev_list.begin();
    while(it != loaded.ble_dev_list.end())
    {
        delete it.value();
        ++it;
    }

    //qDeleteAll(loaded.ble_dev_list.begin(), loaded.ble_dev_list.end());
    loaded.ble_dev_list.clear();
}
