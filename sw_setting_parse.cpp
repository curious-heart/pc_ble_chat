#include <QtXml>
#include <QFile>
#include <QMap>

#include "sw_setting_parse.h"
#include "types_and_defs.h"
#include "diy_common_tool.h"
#include "logger.h"

static inline QString ble_dev_list_elem() { return QStringLiteral("ble_dev_list");}
static inline QString dev_elem() { return QStringLiteral("dev");}
static inline QString addr_elem() { return QStringLiteral("addr");}
static inline QString srv_uuid_elem() { return QStringLiteral("srv_uuid");}
static inline QString rx_char_elem() { return QStringLiteral("rx_char");}
static inline QString tx_char_elem() { return QStringLiteral("tx_char");}
static inline QString lambda_list_elem() {return QStringLiteral("lambda_list");}
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
typedef QMap<QString, parser_t> str_parser_map_t;
static str_parser_map_t xml_parser;
static str_bool_map_t xml_parse_result;
static bool check_setting_device_info(setting_ble_dev_info_t &dev_info)
{
    bool ret = true;
    if(!isMacAddress(dev_info.addr)
        || !isFullUUID(dev_info.srv_uuid)
        || !isFullUUID(dev_info.rx_char_uuid)
        || !isFullUUID(dev_info.tx_char_uuid)
        || dev_info.lambda_list.isEmpty())
    {
        qDebug() << "There is some error in setting_device_info: ";
        ret = false;
    }
    dev_info.log_print();
    return ret;
}

static bool check_setting_db_info(setting_db_info_t &db_info)
{
    bool ret = true;
    if(db_info.srvr_addr.isEmpty()|| (0 == db_info.srvr_port)
            || db_info.db_name.isEmpty())
    {
        qDebug() << "There is some error in setting_db_info: ";
        ret = false;
    }
    db_info.log_print();
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
                else if(info_e.tagName() == lambda_list_elem())
                {
                    QStringList lb = info_e.text().split(',');
                    for(int i =0; i < lb.length(); i++)
                    {
                        bool ok;
                        int val;
                        val = lb.at(i).toInt(&ok);
                        if(ok)
                        {
                            dev_info->lambda_list.append(val);
                        }
                        else
                        {
                            qDebug() << "Invalid value in lambda_list: "
                                     << lb.at(i);
                            dev_info->lambda_list.clear();
                            break;
                        }
                    }
                }
                else
                {
                    qDebug() << "Unknown element " << info_e.tagName() << " found, not processed";
                }
                info_e = info_e.nextSiblingElement();
            }
            //validation of dev_info
            if(check_setting_device_info(*dev_info))
            {
                loaded.ble_dev_list.insert(dev_info->addr, dev_info);
                dev_empty_flag = false;
            }
            else
            {
                delete dev_info;
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

    loaded.db_info.valid = check_setting_db_info(loaded.db_info);
    return loaded.db_info.valid;
}

static bool check_setting_oth_settings(setting_oth_t &oth_info)
{
    return true;
}

static bool parse_oth_settings(QDomElement &e, sw_settings_t &loaded)
{
    QDomElement info_e = e.firstChildElement();
    bool ret = false;
    while(!info_e.isNull())
    {
        if(info_e.tagName() == use_remote_db_elem())
        {
            loaded.oth_settings.use_remote_db = (bool)(info_e.text().toInt());
        }
        else
        {
            QString err;
            err = QString("Unknown xml element in %1:%2").arg(e.tagName()).arg(info_e.tagName());
            Logger::instance()->writeLog(__FILE__, __LINE__,
                                         LOG_LEVEL::LOG_INFO, err);
        }
        info_e = info_e.nextSiblingElement();
    }
    ret = check_setting_oth_settings(loaded.oth_settings);
    return ret;
}

static str_parser_map_t &init_sw_settings_xml_parser()
{
    xml_parser.insert(ble_dev_list_elem(), parse_dev_list);
    xml_parser.insert(db_info_elem(), parse_db_info);
    xml_parser.insert(oth_settings_elem(), parse_oth_settings);

    return xml_parser;
}

static str_bool_map_t& load_sw_settings_from_xml(sw_settings_t &loaded)
{
    QFile xml_file(g_settings_xml_fpn);
    QDomDocument doc;
    QString err;

    str_parser_map_t &parser_map = init_sw_settings_xml_parser();
    xml_parse_result.clear();
    if(!xml_file.open(QIODevice::ReadOnly))
    {
        err = QString("Open XML file %1 fail!").arg(g_settings_xml_fpn);
        Logger::instance()->writeLog(__FILE__, __LINE__, LOG_LEVEL::LOG_WARN, err);
        return xml_parse_result;
    }
    if(!doc.setContent(&xml_file))
    {
        err = QString("DomDocument setContent fail");
        Logger::instance()->writeLog(__FILE__, __LINE__, LOG_LEVEL::LOG_ERROR, err);
        return xml_parse_result;
    }
    xml_file.close();


    QDomElement docElem = doc.documentElement();
    QDomElement c_e = docElem.firstChildElement();
    bool ret = false;
    while(!c_e.isNull())
    {
        ret = false;
        QString key = c_e.tagName();
        parser_t parser = parser_map.value(key, nullptr);
        if(parser)
        {
            ret = parser(c_e, loaded);
        }
        xml_parse_result.insert(key, ret);

        c_e = c_e.nextSiblingElement();
    }
    return xml_parse_result;
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

    loaded.ble_dev_list.clear();
}

void load_sw_settings(sw_settings_t &sw_s)
{
    str_bool_map_t &pt_ret = load_sw_settings_from_xml(sw_s);
    str_bool_map_t::iterator it = pt_ret.begin();

    //xml_parse_result is now filled, check it now:
    while(it != pt_ret.end())
    {
        if(!it.value())
        {/*settings xml file not usable, load default settings.*/
            Logger::instance()->writeLog(__FILE__, __LINE__, LOG_LEVEL::LOG_INFO,
                                         QString("xml文件元素 %1 无效").arg(it.key()));
            if(it.key() == ble_dev_list_elem())
            {
                setting_ble_dev_info_t *ble_dev = new setting_ble_dev_info_t;
                assert(ble_dev != nullptr);
                ble_dev->addr = QString(g_def_ble_dev_addr);
                ble_dev->srv_uuid = QString(g_def_ble_srv_uuid);
                ble_dev->rx_char_uuid = QString(g_def_ble_rx_char_uuid);
                ble_dev->tx_char_uuid = QString(g_def_ble_tx_char_uuid);
                sw_s.ble_dev_list.insert(ble_dev->addr, ble_dev);
            }
            else if(it.key() == db_info_elem())
            {
                sw_s.db_info.srvr_addr = QString(g_def_db_srvr_addr);
                sw_s.db_info.srvr_port = g_def_db_srvr_port;
                sw_s.db_info.db_name = QString(g_def_db_name);
                sw_s.db_info.login_id = QString(g_def_db_login_id);
                sw_s.db_info.login_pwd = QString(g_def_db_login_pwd);
                sw_s.db_info.dbms_name = QString(g_def_dbms_name);
                sw_s.db_info.dbms_ver = QString(g_def_dbms_ver);
                sw_s.db_info.valid = true;
            }
            else if(it.key() == oth_settings_elem())
            {
                sw_s.oth_settings.use_remote_db = g_def_use_remote_db;
            }
        }
        ++it;
    }
}
