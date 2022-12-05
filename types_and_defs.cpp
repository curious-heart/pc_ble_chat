#include "ble_comm_pkt.h"
#include "types_and_defs.h"

/*As below are some default info if there is no valid settings xml file.*/
const char* g_def_ble_dev_addr = "27:A5:D2:1E:67:DD";
const char*  g_def_ble_srv_uuid = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
const char* g_def_ble_tx_char_uuid = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";
const char* g_def_ble_rx_char_uuid = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";

const int g_max_light_flash_period = 100*1000; //100 seconds
const int g_min_light_flash_period = 500; //500 milliseconds
const int g_def_light_flash_period = 500; //500 milliseconds
const int g_max_light_flash_gap = 1000;//1 second
const int g_min_light_flash_gap = 0;
const int g_def_light_flash_gap = 10; //10 milliseconds
const char * g_def_dev_id_prefix = "skin_detc_";
const int g_def_single_light_wait_time = 2*(g_def_light_flash_period + g_def_light_flash_gap);
const int g_min_light_idx_m1 = 0; //minum light idx - 1

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

/*The lambda list needs to be re-initialized into Qt type in cTypeAndDefs_T constructor.*/
static const quint32 g_ori_lambda_list[] = {354,376,405,450,505,560,600,645,680,705,800,850,910,930,970,1000,1050,1070,1200};
static const quint32 g_clone_def_lambda_list[] = {354,376,405,450,505,560,600,645,680,705,800,850,910,930,970,1000,1050,1070,1200};
/*Begin: TypesAndDefs_T*/
TypesAndDefs_T::TypesAndDefs_T()
{
    static int_array_num_t ori = {g_ori_lambda_list, DIY_SIZE_OF_ARRAY(g_ori_lambda_list)};
    dev_type_def_light_map.insert(ORI_DEV_TYPE_STR, ori);

    static int_array_num_t clone = {g_clone_def_lambda_list, DIY_SIZE_OF_ARRAY(g_clone_def_lambda_list)};
    dev_type_def_light_map.insert(CLONE_DEV_TYPE_STR, clone);
}

TypesAndDefs_T::~TypesAndDefs_T()
{
    dev_type_def_light_map.clear();
}
TypesAndDefs_T g_def_values;
/*End: TypesAndDefs_T*/

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
