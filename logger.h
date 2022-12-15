#ifndef LOGGER_H
#define LOGGER_H

/*
 * 从条纹相机上位机程序拷贝修改。
 */

#include <QObject>
#include <QDebug>
#include <QThread>

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

    //static Logger *instance(); //This is not used any more. Refer to comments in logger.cpp file.
public slots:
    void receive_log(LOG_LEVEL level, QString loc_str, QString log_str);

private:
    void writeLog(QString level_str, QString loc_str, QString msg);
};

class LogSigEmitter: public QObject
{
    Q_OBJECT
signals:
    void record_log(LOG_LEVEL level, QString loc_str, QString log_str);
};
extern LogSigEmitter *g_LogSigEmitter;

/*
 * After start_log_thread is invoked with a QThread instance can DIY_LOG work.
 * Note: the life-cycle of th must expands to the whole thread.
 * E.g. you can define a QThread obj in main function, and call start/end_log_thread
 * with that obj.
 *
*/
void start_log_thread(QThread &th);
void end_log_thread(QThread &th);

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
        QString loc_str = QString(__FILE__) + QString("  [%1]").arg(__LINE__);\
        if(g_LogSigEmitter)\
        {\
            emit g_LogSigEmitter->record_log(level, loc_str, log);\
        }\
        qDebug() << (log) << "\n";\
    }
#endif // LOGGER_H
