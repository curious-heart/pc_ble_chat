#ifndef DIY_COMMON_TOOL_H
#define DIY_COMMON_TOOL_H

#include <QByteArray>
#include <QString>
#include <QDateTime>
#include <QDir>
#include <QStringList>

QString QByteHexString(const QByteArray &qba, const QString sep = QString(" "));
QByteArray hex_str_to_byte_array(QString &str);
QString diy_curr_date_time_str_ms(bool with_ms = true);
quint8 diy_crc8_8540(const QByteArray &data, int len);
quint8 diy_crc8_8210(const QByteArray &data, int len);
bool is_mac_address(QString mac);
bool is_full_uuid(QString uuid);
bool mkpth_if_not_exists(QString &pth_str);
void get_dir_content_fpn_list(QString &path, QStringList &result,
                              QDir::Filter filter_f = QDir::Filter::NoFilter,
                              QDir::SortFlag sort_f = QDir::SortFlag::Name,
                              QString ext_name = "");

#define DIY_SIZE_OF_ARRAY(arr) (sizeof(arr)/sizeof((arr)[0]))

typedef class _rdb_info_t
{
public:
    bool valid;
    QString srvr_addr;
    uint16_t srvr_port;
    QString db_name, login_id, login_pwd;
    QString dbms_name, dbms_ver;

    _rdb_info_t()
    {
        clear();
    }
    ~_rdb_info_t()
    {
        clear();
    }
    void clear()
    {
        srvr_addr = db_name = login_id = login_pwd = dbms_name = dbms_ver = QString();
        srvr_port = 0;
        valid = false;
    }
    QString log_print(bool dbg = true);
}setting_rdb_info_t;
#endif // DIY_COMMON_TOOL_H
