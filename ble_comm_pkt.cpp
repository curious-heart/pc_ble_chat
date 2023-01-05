#include <QString>
#include <QByteArray>

#include "diy_common_tool.h"
#include "ble_comm_pkt.h"
#include "logger.h"

static bool ble_comm_gen_ori_dev_app_light_pkt(QByteArray &qba, int idx)
{
    static const int PWD_CARDINALITY[] = {35, 35, 40, 100, 45, 35, 80, 45, 40, 40, 60, 45, 10, 35, 85, 83, 85, 90, 90};
    static const int pkt_len_without_crc = 19;
    if(idx < 1 || idx > DIY_SIZE_OF_ARRAY(PWD_CARDINALITY))
    {
        DIY_LOG(LOG_LEVEL::LOG_ERROR, "Bad idx:%d", idx);
        return false;
    }

    qba.clear();
    qba += ORID_APP_LIGHT_PKT_HDR;
    qba.append((char)idx);
    qba.append((char)PWD_CARDINALITY[idx - 1]);
    qba.append((char)1);
    int curr_len = qba.count();
    qba.append(pkt_len_without_crc - curr_len, (char)0);
    quint8 data_crc8 = diy_crc8_8210(qba,qba.count());
    qba.append((char)data_crc8);

    return true;
}

static bool ble_comm_gen_clone_dev_app_light_pkt(QByteArray &qba, int idx)
{
    return ble_comm_gen_ori_dev_app_light_pkt(qba, idx);
}

bool ble_comm_gen_app_light_pkt(QByteArray &qba, int idx, QString dev_type)
{
    bool ret = true;
    if(ORI_DEV_TYPE_STR == dev_type)
    {
        ret = ble_comm_gen_ori_dev_app_light_pkt(qba, idx);
    }
    else if(CLONE_DEV_TYPE_STR == dev_type)
    {
        ret = ble_comm_gen_clone_dev_app_light_pkt(qba, idx);
    }
    else
    {
        DIY_LOG(LOG_LEVEL::LOG_WARN, "Unknown device type: %ls", dev_type.utf16());
        ret = false;
    }

    return ret;
}
/* qba: containing hex digit chars, e.g. "5a0112"
*/
bool ble_comm_get_lambda_data_from_pkt(QByteArray &qba, quint32 &lambda, quint64 &data,
                                       QString /*dev_type*/)
{
    if(qba.isEmpty() || qba.length() < qMax(ORID_DATA_POS_START + ORID_DATA_BYTES_NUM,
                                            ORID_LAMBDA_POS_START + ORID_LAMBDA_BYTES_NUM) * 2)
    {
        return false;
    }

    lambda = QByteArray(&qba[ORID_LAMBDA_POS_START * 2], ORID_LAMBDA_BYTES_NUM * 2).toUInt(nullptr, 16);
    data = QByteArray(&qba[ORID_DATA_POS_START * 2], ORID_DATA_BYTES_NUM * 2).toULongLong(nullptr, 16);

    return true;
}
