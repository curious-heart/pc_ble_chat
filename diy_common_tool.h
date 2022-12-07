#ifndef DIY_COMMON_TOOL_H
#define DIY_COMMON_TOOL_H

#include <QByteArray>
#include <QString>
#include <QDateTime>

QString QByteHexString(const QByteArray &qba, const QString sep = QString(" "));
QString diy_curr_date_time_str_ms();
quint8 diy_crc8_8540(const QByteArray &data, int len);
quint8 diy_crc8_8210(const QByteArray &data, int len);
bool is_mac_address(QString mac);
bool is_full_uuid(QString uuid);
bool mkpth_if_not_exists(QString &pth_str);

#define DIY_SIZE_OF_ARRAY(arr) (sizeof(arr)/sizeof((arr)[0]))

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
    ~_db_info_t()
    {
        clear();
    }
    void clear()
    {
        srvr_addr = db_name = login_id = login_pwd = dbms_name = dbms_ver = QString();
        srvr_port = 0;
        valid = false;
    }
    void log_print();
}setting_db_info_t;
#endif // DIY_COMMON_TOOL_H
