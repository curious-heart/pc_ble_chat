#ifndef DIY_COMMON_TOOL_H
#define DIY_COMMON_TOOL_H

#include <QByteArray>
#include <QString>
#include <QDateTime>

QString QByteHexString(const QByteArray &qba);
QString diy_curr_date_time_str_ms();
quint8 diy_crc8_8540(const QByteArray &data, int len);
quint8 diy_crc8_8210(const QByteArray &data, int len);
#endif // DIY_COMMON_TOOL_H
