#ifndef LOGGER_H
#define LOGGER_H

/*
 * 从条纹相机上位机程序拷贝修改。
 */

#include <QObject>

enum LOG_LEVEL {
    LOG_DEBUG=0,//调试
    LOG_INFO,   //信息
    LOG_WARN,   //警告
    LOG_ERROR   //错误
};

class Logger : public QObject
{
    Q_OBJECT
public:
    explicit Logger(QObject *parent = nullptr);

    static Logger *instance();                                              //获得单例对象
    void writeLog(QString fileName,int lineNo,LOG_LEVEL level,QString log); //写入日志
};

#define DIY_LOG(level, fmt_str, ...) \
    {\
        QString log = QString::asprintf(fmt_str, ##__VA_ARGS__);\
        Logger::instance()->writeLog(__FILE__, __LINE__, (level), (log)); \
        qDebug() << (log) << "\n";\
    }
#endif // LOGGER_H
