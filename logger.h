﻿#ifndef LOGGER_H
#define LOGGER_H

/*
 * 从条纹相机上位机程序拷贝修改。
 */

#include <QObject>
#include <QDebug>
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
public slots:
    void receive_log(QString loc_str, QString log_str);
};

/*
 * Use example:
 *     DIY_LOG(LOG_LEVEL::LOG_INFO, "info str.");
 *     DIY_LOG(LOG_LEVEL::LOG_ERROR, "error code:%d", (int)err);
 *
 *     QString ws("warning...");
 *     DIY_LOG(LOG_LEVEL::LOG_WARN, "warn message:%ls", ws.utf16());
*/
#define DIY_LOG(level, fmt_str, ...) \
    {\
        QString log = QString::asprintf(fmt_str, ##__VA_ARGS__);\
        QString loc_str = QString(__FILE__) + QString(" [%1]").arg(__LINE__);\
        emit record_log(loc_str, log);\
        /*Logger::instance()->writeLog(__FILE__, __LINE__, (level), (log)); */\
        qDebug() << (log) << "\n";\
    }
#endif // LOGGER_H
