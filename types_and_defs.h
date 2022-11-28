#ifndef TYPES_AND_DEFS_H
#define TYPES_AND_DEFS_H

#include <QStringList>
#include <QString>
#include <QMap>
#include <QList>
#include <QByteArray>
#include "diy_common_tool.h"
#include "ble_comm_pkt.h"

extern QStringList g_skin_type;
extern QStringList g_sample_pos;

extern const char * g_def_ble_dev_addr;
extern const char * g_def_ble_srv_uuid;
extern const char * g_def_ble_tx_char_uuid;
extern const char * g_def_ble_rx_char_uuid;
extern const char* g_def_db_srvr_addr;
extern const uint16_t  g_def_db_srvr_port ;
extern const char* g_def_dbms_name;
extern const char* g_def_dbms_ver;
extern const char* g_def_db_name;
extern const char* g_def_db_login_id;
extern const char* g_def_db_login_pwd;
extern const bool g_def_use_remote_db;
extern const char* g_settings_xml_fpn;
extern const char * g_def_dev_id_prefix;
extern const int g_def_single_light_wait_time;
extern const int g_min_light_idx_m1;

extern const int g_max_light_flash_period;
extern const int g_min_light_flash_period;
extern const int g_def_light_flash_period;
extern const int g_max_light_flash_gap;
extern const int g_min_light_flash_gap;
extern const int g_def_light_flash_gap;

typedef struct _int_array_num_t
{
    const int * d_arr;
    int num;
}int_array_num_t;

class TypesAndDefs_T
{
public:
    QMap<QString, int_array_num_t> dev_type_def_light_map;

    TypesAndDefs_T();
    ~TypesAndDefs_T();
};

extern TypesAndDefs_T g_def_values;

#endif // TYPES_AND_DEFS_H
