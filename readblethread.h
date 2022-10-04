#ifndef READBLETHREAD_H
#define READBLETHREAD_H

#include <QThread>
#include <QByteArray>

#include "serviceinfo.h"
#include "characteristicinfo.h"
#include "diy_common_tool.h"

class ReadBLEThread : public QThread
{
    Q_OBJECT
public:
    explicit ReadBLEThread(QObject *parent = nullptr);

    //定义虚函数run()用来执行线程
protected:
    virtual void run();

private slots:
    void BleServiceCharacteristicRead(const QLowEnergyCharacteristic &c,
                                            const QByteArray &value);
    void BleServiceCharacteristicChanged(const QLowEnergyCharacteristic &c,
                                            const QByteArray &value);
private:
    ServiceInfo * m_service= nullptr;
    CharacteristicInfo* m_rx_char = nullptr;
public:
    void SetReadHandle(ServiceInfo* srv, CharacteristicInfo * char_info);
};

#endif // READBLETHREAD_H
