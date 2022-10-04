#include "readblethread.h"

ReadBLEThread::ReadBLEThread(QObject *parent)
    : QThread{parent}
{

}

void ReadBLEThread::SetReadHandle(ServiceInfo* srv, CharacteristicInfo * char_info)
{
    m_service = srv;
    m_rx_char = char_info;
    connect(m_service->service(), &QLowEnergyService::characteristicRead,
            this, &ReadBLEThread::BleServiceCharacteristicRead);
    connect(m_service->service(), &QLowEnergyService::characteristicChanged,
            this, &ReadBLEThread::BleServiceCharacteristicChanged);
}

void ReadBLEThread::run()
{
    if(m_service)
    {
        m_service->service()->readCharacteristic(m_rx_char->getCharacteristic());
    }
}

void ReadBLEThread::BleServiceCharacteristicRead(const QLowEnergyCharacteristic &c,
                                        const QByteArray &value)
{
    QString read_data_str;
    read_data_str = QByteHexString(value);
    qDebug() << "Read data:" << read_data_str;
}

void ReadBLEThread::BleServiceCharacteristicChanged(const QLowEnergyCharacteristic &c,
                                        const QByteArray &value)
{
    qDebug() << "Char Changed:" << QByteHexString(value);
}
