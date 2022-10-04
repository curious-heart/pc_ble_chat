#include "diy_common_tool.h"

QString QByteHexString(const QByteArray &qba)
{
    QString str = "";
    for(int j =0; j < qba.size(); j++)
    {
        str += QString("%1 ").arg((unsigned char)qba[j], 2, 16, QLatin1Char('0'));
    }
    return str;
}

QString diy_curr_date_time_str_ms()
{
    QDateTime curr_date_time;

    curr_date_time = QDateTime::currentDateTime();
    return curr_date_time.toString("yyyyMMdd-hhmmss-zzz");
}
