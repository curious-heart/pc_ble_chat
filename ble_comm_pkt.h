#ifndef BLE_COMM_PKT_H
#define BLE_COMM_PKT_H

#include <QByteArray>

/*Prefix "ORID_" indicates the pkt format of original device.*/
#define ORID_W_ACK_HDR QByteArray("\x5A\xFF\x11\x00")
#define ORID_DATA_PKT_HDR QByteArray("\x5A\x11")
#define ORID_W_FINISH_HDR QByteArray("\x5A\xFF\x11\x02")

#endif // BLE_COMM_PKT_H
