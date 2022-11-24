#ifndef DIY_COMMON_TOOL_H
#define DIY_COMMON_TOOL_H

#include <QByteArray>
#include <QString>
#include <QDateTime>

QString QByteHexString(const QByteArray &qba);
QString diy_curr_date_time_str_ms();
quint8 diy_crc8_8540(const QByteArray &data, int len);
quint8 diy_crc8_8210(const QByteArray &data, int len);
bool is_mac_address(QString mac);
bool is_full_uuid(QString uuid);
bool mkpth_if_not_exists(QString &pth_str);

#define DIY_SIZE_OF_ARRAY(arr) (sizeof(arr)/sizeof((arr)[0]))
#endif // DIY_COMMON_TOOL_H
