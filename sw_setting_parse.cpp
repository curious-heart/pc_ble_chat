#include <QtXml>
#include <QFile>
#include <QMap>
#include <QMessageBox>

#include "logger.h"
#include "sw_setting_parse.h"
#include "types_and_defs.h"
#include "diy_common_tool.h"
#include "ble_comm_pkt.h"

static inline QString ble_dev_list_elem() { return QStringLiteral("ble_dev_list");}
static inline QString dev_elem() { return QStringLiteral("dev");}
static inline QString dev_type_elem() { return QStringLiteral("dev_type");}
static inline QString addr_elem() { return QStringLiteral("addr");}
static inline QString srv_uuid_elem() { return QStringLiteral("srv_uuid");}
static inline QString rx_char_elem() { return QStringLiteral("rx_char");}
static inline QString tx_char_elem() { return QStringLiteral("tx_char");}
static inline QString light_list_elem() {return QStringLiteral("light_list");}
static inline QString light_elem() {return QStringLiteral("light");}
static inline QString lambda_elem() {return QStringLiteral("lambda");}
static inline QString flash_period_elem() { return QStringLiteral("flash_period");}
static inline QString flash_gap_elem() { return QStringLiteral("flash_gap");}
static inline QString idx_elem() { return QStringLiteral("idx");}
static inline QString light_count_elem() { return QStringLiteral("light_count");}
static inline QString single_light_wait_time_elem() { return QStringLiteral("single_light_wait_time");}

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

typedef bool (*parser_t)(QDomElement &n, sw_settings_t &loaded, bool &valid_e);
typedef QMap<QString, parser_t> str_parser_map_t;
static bool check_setting_device_info(setting_ble_dev_info_t &dev_info)
{
    bool ret = true;
    if(!is_mac_address(dev_info.addr))
    {
        DIY_LOG(LOG_LEVEL::LOG_WARN,
                "%ls error in setting_device_info: %ls",
                addr_elem().utf16(), dev_info.addr.utf16());
        ret = false;
    }
    if(!is_full_uuid(dev_info.srv_uuid))
    {
        DIY_LOG(LOG_LEVEL::LOG_WARN,
                "%ls error in setting_device_info: %ls",
                srv_uuid_elem().utf16(), dev_info.srv_uuid.utf16());
        ret = false;
    }
    if(!is_full_uuid(dev_info.rx_char_uuid))
    {
        DIY_LOG(LOG_LEVEL::LOG_WARN,
                "%ls error in setting_device_info: %ls",
                rx_char_elem().utf16(), dev_info.rx_char_uuid.utf16());
        ret = false;
    }
    if(!is_full_uuid(dev_info.tx_char_uuid))
    {
        DIY_LOG(LOG_LEVEL::LOG_WARN,
                "%ls error in setting_device_info: %ls",
                tx_char_elem().utf16(), dev_info.tx_char_uuid.utf16());
        ret = false;
    }
    return ret;
}

static bool check_setting_db_info(setting_rdb_info_t &db_info)
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

static bool parse_light_list(QDomElement &e, light_list_t & light_list, bool &valid)
{
    /*
     * 解析xml文件中light_list元素下的灯信息.
     * @param e: <light_list>...</light_list>
     * @param light_list: 返回的解析结果。
     *
     * @return
     * true: 无错误；或者只存在参数非法类错误，已用默认值替代。
     * false：not true。
     * 注意：返回false时，light_list 中已经插入的值在调用者中释放。
     * valid返回：是否存在非法值。
    */
    QDomElement light_e = e.firstChildElement();
    bool ok, ret = true;
    bool exist_info_e = false;
    light_info_t in_store;
    valid = true;
    while(!light_e.isNull())
    {
        if(light_e.tagName() != light_elem())
        {
            DIY_LOG(LOG_LEVEL::LOG_WARN, "Unknown element %ls under %s. Ignored.",
                    light_e.tagName().utf16(), light_list_elem().utf16());
            light_e = light_e.nextSiblingElement();
            continue;
        }

        exist_info_e = false;
        in_store.flash_gap = g_def_light_flash_gap;
        in_store.flash_period = g_def_light_flash_period;
        in_store.idx = g_min_light_idx_m1;
        in_store.lambda = 0;
        QDomElement info_e = light_e.firstChildElement();
        while(!info_e.isNull())
        {
            ok = false;
            if(info_e.tagName() == lambda_elem())
            {
                quint32 lambda_i;
                lambda_i = info_e.text().toUInt(&ok);
                if(ok)
                {
                    in_store.lambda = lambda_i;
                }
                else
                {
                    DIY_LOG(LOG_LEVEL::LOG_ERROR,
                            "Invalid %ls: %ls. Must be int. Now it is taken as 0!",
                            info_e.tagName().utf16(), info_e.text().utf16());
                    valid = false;
                }
                exist_info_e = true;
            }
            else if(info_e.tagName() == flash_period_elem())
            {
                int period_i;
                period_i = info_e.text().toInt(&ok);
                if(ok && (period_i <= g_max_light_flash_period)
                       && (period_i >= g_min_light_flash_period))
                {
                    in_store.flash_period = period_i;
                }
                else
                {
                   DIY_LOG(LOG_LEVEL::LOG_WARN,
                           "Invlaid %ls: %ls, should be in [%d, %d]. set to default value %d",
                           info_e.tagName().utf16(), info_e.text().utf16(),
                           g_min_light_flash_period,
                           g_max_light_flash_period,
                           g_def_light_flash_period);

                    valid = false;
                }
                exist_info_e = true;
            }
            else if(info_e.tagName() == flash_gap_elem())
            {
                int gap_i;
                gap_i = info_e.text().toInt(&ok);
                if(ok && (gap_i <= g_max_light_flash_gap)
                       && (gap_i >= g_min_light_flash_gap))
                {
                    in_store.flash_gap = gap_i;
                }
                else
                {
                    DIY_LOG(LOG_LEVEL::LOG_WARN,
                            "Invlaid %ls: %ls, should be in [%d, %d]. set to default value %d",
                            info_e.tagName().utf16(), info_e.text().utf16(),
                            g_min_light_flash_gap,
                            g_max_light_flash_gap,
                            g_def_light_flash_gap);
                    valid = false;
                }
                exist_info_e = true;
            }
            else if(info_e.tagName() == idx_elem())
            {
                int idx_i;
                idx_i = info_e.text().toInt(&ok);
                if(ok && (idx_i >= g_min_light_idx_m1 + 1))
                {
                    in_store.idx = idx_i;
                }
                else
                {
                    DIY_LOG(LOG_LEVEL::LOG_WARN,
                            "Invlaid %ls: %ls. Must be int and >= %d."
                            "All invalid values are taken as 1-step-increased value from its former valid one.",
                            info_e.tagName().utf16(), info_e.text().utf16(),
                            g_min_light_idx_m1 + 1);
                    valid = false;
                }
                exist_info_e = true;
            }
            else
            {
                DIY_LOG(LOG_LEVEL::LOG_WARN, "Unknown element: %ls, ignored",
                        info_e.tagName().utf16());
            }

            info_e = info_e.nextSiblingElement();
        }

        if(exist_info_e)
        {
            light_list.append(in_store);
        }

        light_e = light_e.nextSiblingElement();
    }
    return ret;
}

static void take_default_lights(light_list_t &light_list, QString &dev_type)
{
    int_array_num_t *def_lights;
    if(!g_def_values.dev_type_def_light_map.contains(dev_type))
    {
        DIY_LOG(LOG_LEVEL::LOG_WARN, "Unknown %ls:%ls, use default:%ls",
                dev_type_elem().utf16(), dev_type.utf16(), CLONE_DEV_TYPE_STR.utf16());
        def_lights = &(g_def_values.dev_type_def_light_map[CLONE_DEV_TYPE_STR]);
    }
    else
    {
        def_lights = &(g_def_values.dev_type_def_light_map[dev_type]);
    }
    for(int i = 0; i < def_lights->num; i++)
    {
        light_info_t in;
        /*if user does not assign lambda in xml, use the value from device,
         * so we set lambda to 0 here.
         * Refer to the use of lambda in BleServiceCharacteristicChanged
        */
        in.lambda = 0; //def_lights->d_arr[i];
        in.flash_period = g_def_light_flash_period;
        in.flash_gap= g_def_light_flash_gap;
        in.idx = i+1;

        light_list.append(in);
    }
}

/*
 * Return:
 *  true: there are some valid dev element parsed.
 *  false: no valid dev element is parsed and insert into loaded (because xml
 *  contains no valid info, or some other fatal error).
 *
*/
static bool parse_dev_list(QDomElement &e, sw_settings_t &loaded, bool &valid_e)
{
    QDomElement dev_e = e.firstChildElement();
    bool dev_empty_flag = true, valid_light_list = true;
    valid_e = true;
    while(!dev_e.isNull())
    {
        if(dev_e.tagName() == dev_elem())
        {
            setting_ble_dev_info_t * dev_info = new setting_ble_dev_info_t;
            if(!dev_info)
            {
                DIY_LOG(LOG_LEVEL::LOG_ERROR,
                        "New setting_ble_dev_info_t fail! Stop parsing dev elements.");
                return !dev_empty_flag;
            }
            QDomElement info_e = dev_e.firstChildElement();
            while(!info_e.isNull())
            {
                if(info_e.tagName() == addr_elem())
                {
                    dev_info->addr = info_e.text();
                }
                else if(info_e.tagName() == dev_type_elem())
                {
                    dev_info->dev_type = info_e.text();
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
                else if(info_e.tagName() == light_list_elem())
                {
                    valid_light_list = parse_light_list(info_e, dev_info->light_list, valid_e);
                    if(!valid_light_list)
                    {
                        dev_info->clear_light_list();
                    }
                }
                else if(info_e.tagName() == single_light_wait_time_elem())
                {
                    bool ok;
                    dev_info->single_light_wait_time = info_e.text().toInt(&ok);
                    if(!ok)
                    {
                        dev_info->single_light_wait_time = g_def_single_light_wait_time;
                        valid_e = false;
                    }
                }
                else if(info_e.tagName() == light_count_elem())
                {
                    bool ok;
                    dev_info->light_cnt = info_e.text().toInt(&ok);
                    if(!ok)
                    {
                        dev_info->light_cnt = 0;
                        valid_e = false;
                    }
                }
                else
                {
                    DIY_LOG(LOG_LEVEL::LOG_INFO,
                            "Unknown element %ls, found, not processed",
                            info_e.tagName().utf16());
                }
                info_e = info_e.nextSiblingElement();
            }
            //validation of dev_info
            if(check_setting_device_info(*dev_info) && valid_light_list)
            {
                if(dev_info->dev_type.isEmpty())
                {
                    DIY_LOG(LOG_LEVEL::LOG_INFO,
                            "xml: device %ls, device type is empty, use %ls",
                            dev_info->addr.utf16(),
                            CLONE_DEV_TYPE_STR.utf16());
                    dev_info->dev_type = CLONE_DEV_TYPE_STR;
                }

                if((dev_info->light_cnt <= 0) && dev_info->light_list.isEmpty())
                {//fill in default lights.
                    DIY_LOG(LOG_LEVEL::LOG_INFO,
                            "Light is is taken as empty in xml file, use default light list.");
                    take_default_lights(dev_info->light_list, dev_info->dev_type);
                    dev_info->light_cnt = dev_info->light_list.count();
                }

                loaded.ble_dev_list.insert(dev_info->addr, dev_info);
                dev_empty_flag = false;
            }
            else
            {
                DIY_LOG(LOG_LEVEL::LOG_WARN,
                        "Invalid device information in %ls, or soem fatal error exists."
                        "This device is ignored.",
                        dev_info->addr.utf16());
                delete dev_info;
            }
        }
        else
        {
            DIY_LOG(LOG_LEVEL::LOG_WARN,
                    "Unknown element in device list: %ls.",
                    dev_e.tagName().utf16());
        }
        dev_e = dev_e.nextSiblingElement();
    }
    if(dev_empty_flag)
    {
        DIY_LOG(LOG_LEVEL::LOG_WARN,
                "No valid device infomation element found! Use default device:\n"
                "addr: %s\n, srv_uuid: %s\n, tx_char: %s, rx_char: %s\n",
                g_def_ble_dev_addr,
                g_def_ble_srv_uuid,
                g_def_ble_tx_char_uuid,
                g_def_ble_rx_char_uuid);
    }

    return !dev_empty_flag;
}

static bool parse_db_info(QDomElement &e, sw_settings_t &loaded, bool &valid_e)
{
    QDomElement info_e = e.firstChildElement();

    valid_e = true;
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

static bool check_setting_oth_settings(setting_oth_t &/*oth_info*/)
{
    return true;
}

static bool parse_oth_settings(QDomElement &e, sw_settings_t &loaded, bool &valid_e)
{
    QDomElement info_e = e.firstChildElement();
    bool ret = false;
    valid_e = true;
    while(!info_e.isNull())
    {
        if(info_e.tagName() == use_remote_db_elem())
        {
            loaded.oth_settings.use_remote_db = (bool)(info_e.text().toInt());
        }
        else
        {
            DIY_LOG(LOG_LEVEL::LOG_INFO,
                    "Unknown xml element in %ls:%ls",
                    e.tagName().utf16(),info_e.tagName().utf16());
        }
        info_e = info_e.nextSiblingElement();
    }
    ret = check_setting_oth_settings(loaded.oth_settings);
    return ret;
}

static str_parser_map_t &init_sw_settings_xml_parser(str_bool_map_t & ret)
{
    static str_parser_map_t xml_parser;
    xml_parser.clear();
    xml_parser.insert(ble_dev_list_elem(), parse_dev_list);
    xml_parser.insert(db_info_elem(), parse_db_info);
    xml_parser.insert(oth_settings_elem(), parse_oth_settings);

    ret.clear();
    ret.insert(ble_dev_list_elem(), false);
    ret.insert(db_info_elem(), false);
    ret.insert(oth_settings_elem(), false);
    return xml_parser;
}

static str_bool_map_t& load_sw_settings_from_xml(sw_settings_t &loaded, bool& valid_e)
{
    QFile xml_file(g_settings_xml_fpn);
    QString err_str, xml_abs_pth;
    QDomDocument doc;
    QString xml_err_str;
    int xml_err_line, xml_err_col;
    static str_bool_map_t xml_parse_result;

    QFileInfo fi(xml_file);
    xml_abs_pth = fi.absoluteFilePath();
    DIY_LOG(LOG_LEVEL::LOG_INFO, "load xml: %ls", xml_abs_pth.utf16());

    str_parser_map_t &parser_map = init_sw_settings_xml_parser(xml_parse_result);
    if(!xml_file.open(QIODevice::ReadOnly))
    {
        err_str = QString("打开XML文件失败！\n") + xml_abs_pth
                  + "\n\n使用软件内部默认配置。";
        DIY_LOG(LOG_LEVEL::LOG_WARN, "%ls", err_str.utf16());
        QMessageBox::critical(nullptr, "!!!", err_str);
        return xml_parse_result;
    }
    if(!doc.setContent(&xml_file, &xml_err_str, &xml_err_line, &xml_err_col))
    {
        err_str = QString("DomDocument setContent失败！\n") + xml_abs_pth
                + QString("\n错误信息：\n%1 ").arg(xml_err_str)
                + QString("Line: %1，").arg(xml_err_line)
                + QString("Col: %1\n").arg(xml_err_col)
                + "\n\n使用软件内部默认配置。";
        DIY_LOG(LOG_LEVEL::LOG_ERROR, "%ls", err_str.utf16());
        QMessageBox::critical(nullptr, "!!!", err_str);
        return xml_parse_result;
    }
    xml_file.close();

    QDomElement docElem = doc.documentElement();
    QDomElement c_e = docElem.firstChildElement();
    bool ret = false;
    bool valid_one_e = true;
    valid_e = true;
    while(!c_e.isNull())
    {
        ret = false;
        QString key = c_e.tagName();
        parser_t parser = parser_map.value(key, nullptr);
        if(parser)
        {
            ret = parser(c_e, loaded, valid_one_e);
            valid_e = valid_e && valid_one_e;
        }
        xml_parse_result[key] = ret;

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

static void insert_def_dev_item(sw_settings_t &sw_s)
{
    setting_ble_dev_info_t *ble_dev = new setting_ble_dev_info_t;
    assert(ble_dev != nullptr);
    ble_dev->addr = QString(g_def_ble_dev_addr);
    ble_dev->srv_uuid = QString(g_def_ble_srv_uuid);
    ble_dev->rx_char_uuid = QString(g_def_ble_rx_char_uuid);
    ble_dev->tx_char_uuid = QString(g_def_ble_tx_char_uuid);
    ble_dev->dev_type = CLONE_DEV_TYPE_STR;
    take_default_lights(ble_dev->light_list, ble_dev->dev_type);
    ble_dev->light_cnt = ble_dev->light_list.count();
    ble_dev->single_light_wait_time = g_def_single_light_wait_time;
    sw_s.ble_dev_list.insert(ble_dev->addr, ble_dev);
}

bool load_sw_settings(sw_settings_t &sw_s, bool &valid_e)
{
    bool ret = true;
    str_bool_map_t &pt_ret = load_sw_settings_from_xml(sw_s, valid_e);
    str_bool_map_t::iterator it = pt_ret.begin();

    //xml_parse_result is now filled, check it now:
    while(it != pt_ret.end())
    {
        if(!it.value())
        {/*settings xml file not usable, load default settings.*/
            DIY_LOG(LOG_LEVEL::LOG_INFO, "xml文件元素 %ls 无效",it.key().utf16());
            if(it.key() == ble_dev_list_elem())
            {
                insert_def_dev_item(sw_s);
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
            ret = false;
        }
        ++it;
    }
    /* The following call of inset_def_dev_item is to guarantee that the item with
     * g_def_ble_dev_addr address is always in list, so that when there is no xml file
     * or the user device address is not recorded in xml, the programm can work.
     * See on_remoteDevices_itemActivated function in remoteselector.cpp.
     */
    setting_ble_dev_info_t * def_dev_item = sw_s.ble_dev_list.value(g_def_ble_dev_addr,
                                                                    nullptr);
    if(nullptr == def_dev_item)
    {
        insert_def_dev_item(sw_s);
    }

    //check single_light_wait_time and fill light idx.
    QMap<QString, setting_ble_dev_info_t*>::Iterator dev_it = sw_s.ble_dev_list.begin();
    light_list_t::Iterator light_it;
    int w_t, cur_v, pre_idx ;
    while(dev_it != sw_s.ble_dev_list.end())
    {
        light_it = dev_it.value()->light_list.begin();
        w_t = dev_it.value()->single_light_wait_time;
        pre_idx = g_min_light_idx_m1;
        while(light_it != dev_it.value()->light_list.end())
        {
            cur_v = (light_it->flash_period + light_it->flash_gap) * 2;
            if(w_t < cur_v) { w_t = cur_v;}
            if(light_it->idx <= g_min_light_idx_m1)
            {
                light_it->idx = pre_idx + 1;
            }
            pre_idx = light_it->idx;

            ++light_it;
        }
        dev_it.value()->single_light_wait_time = w_t;

        int need_add = dev_it.value()->light_cnt - dev_it.value()->light_list.count();
        int last_idx =
                (dev_it.value()->light_list.isEmpty()) ? g_min_light_idx_m1 + 1 : dev_it.value()->light_list.last().idx + 1;
        if(need_add > 0)
        {
            for(int idx=0; idx < need_add; idx++)
            {
                light_info_t in;
                in.lambda = 0;
                in.flash_period = g_def_light_flash_period;
                in.flash_gap= g_def_light_flash_gap;
                in.idx = last_idx + idx;

                dev_it.value()->light_list.append(in);
            }
        }
        else
        {
            dev_it.value()->light_cnt = dev_it.value()->light_list.count();
        }
        ++dev_it;
    }

    return ret;
}
