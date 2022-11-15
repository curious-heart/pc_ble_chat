#include "types_and_defs.h"

/*As below are some default info if there is no valid settings xml file.*/
const char* g_def_ble_dev_addr = "27:A5:D2:1E:67:DD";
const char*  g_def_ble_srv_uuid = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
const char* g_def_ble_tx_char_uuid = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";
const char* g_def_ble_rx_char_uuid = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";

const char* g_def_db_srvr_addr = "li371359001.vicp.net";
const uint16_t  g_def_db_srvr_port = 3306;
const char* g_def_dbms_name = "MySQL";
const char* g_def_dbms_ver = "5.7.26";
const char* g_def_db_name = "detector";
const char* g_def_db_login_id = "admin";
const char* g_def_db_login_pwd = "admin123";

const bool g_def_use_remote_db = true;

/*sw settings.xml*/
const char* g_settings_xml_fpn = "./settings/settings.xml";

/*As below are skint experiment related info.*/
QStringList g_skin_type = {
    "痤疮型",
    "干燥型",
    "敏感型",
    "色沉型",
    "皱纹型",
};

QStringList g_sample_pos = {
"额头",
"左脸颊",
"右脸颊",
"左下腮",
"右下腮",
"左嘴角",
"右嘴角",
"下巴",
"左下巴",
"右下巴",
"左内眼角",
"左外眼角",
"右内眼角",
"右外眼角",
"左外眼周",
"右外眼周",
"眉间",
"眉心",
"左鼻子",
"右鼻子",
"左脖子",
"右脖子",
"左法令纹",
"右法令纹",
"左下颌",
"右下颌",
};