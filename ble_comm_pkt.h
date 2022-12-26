#ifndef BLE_COMM_PKT_H
#define BLE_COMM_PKT_H

#include <QString>
#include <QByteArray>

#define ORI_DEV_TYPE_STR QString("original_dev")
/*Prefix "ORID_" indicates the pkt format of original device.*/
#define ORID_APP_LIGHT_PKT_HDR QByteArray("\x3D\x11")
#define ORID_DEV_RESP_PKT_HDR QByteArray("\x5A\xFF\x11\x00")
#define ORID_DEV_DATA_PKT_HDR QByteArray("\x5A\x11")
#define ORID_DEV_FINISH_PKT_HDR QByteArray("\x5A\xFF\x11\x02")
#define ORID_DATA_POS_START 8
#define ORID_DATA_BYTES_NUM 3
#define ORID_LAMBDA_POS_START 3
#define ORID_LAMBDA_BYTES_NUM 2

#define CLONE_DEV_TYPE_STR QString("clone_dev")

/*idx: used on ble interface. On original device, it is from 1 to 19.
 * DON'T confuse it with the light count in this APP file.
*/
bool ble_comm_gen_app_light_pkt(QByteArray &qba, int idx, QString dev_type);
#endif // BLE_COMM_PKT_H
