#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>
#include <QString>

#include "logger.h"

LogSigEmitter *g_LogSigEmitter = nullptr;
static Logger * g_LogWorker = nullptr;

Logger::Logger(QObject *parent) : QObject(parent)
{

}

/*
 * According to Qt help manual
 * [
 *  Threads and QObjects:
 *  In general, creating QObjects before the QApplication is not supported and can
 *  lead to weird crashes on exit, depending on the platform. This means static instances
 *  of QObject are also not supported. A properly structured single or multi-threaded
 *  application should make the QApplication be the first created, and last destroyed
 *  QObject.
 *  ]
 *  This may applies to GUI classes. But for safety, we decide to obey this rule strictly.
 *  So we do not use static Logger any more...
 *  For the same reason, we define g_LogSigEmitter as a pointer and new an object for it
 *  after QApplication object is created in main function.
Logger *Logger::instance()
{
    static Logger logger;
    return &logger;
}
*/

/**
 * @param level_str 级别字符串
 * @param loc_str 位置字符串，包括文件名和行号
 * @param log 日志内容
 */
void Logger::writeLog(QString level_str, QString loc_str, QString msg)
{
    static const char* log_dir_str = "log", *log_file_str = "log";
    QFileInfo info(log_dir_str);
    QString path = info.absoluteFilePath();
    QDir dir(path);
    if(!dir.exists())
        dir.mkpath(path);
    QString date=QDateTime::currentDateTime().toString("yyyy-MM-dd");
    QString time=QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    QFile file(QString(log_dir_str) + QString("/")
               + QString(log_file_str) + QString("_%1.txt").arg(date));
    if(!file.open(QFile::WriteOnly | QFile::Append))
        return;
    QTextStream in(&file);
    in<<"\t"<<date<<" "<<time<<"  " << loc_str << "  " << level_str << "\n";
    in<<msg<<"\n";
    file.flush();
    file.close();
}
void Logger::receive_log(LOG_LEVEL level, QString loc_str, QString log_str)
{
    static const char *levels[] = {"DEBUG", "INFO", "WARN", "ERROR"};
    QString level_str;
    if((quint32)level >= (quint32)(sizeof(levels)/sizeof(levels[0])))
    {
        level_str = QString("[%1]").arg("Unknown Level");
    }
    else
    {
        level_str = QString("[%1]").arg(levels[level]);
    }
    writeLog(level_str, loc_str, log_str);
}

bool start_log_thread(QThread &th)
{
    g_LogSigEmitter = new LogSigEmitter;
    g_LogWorker = new Logger;
    if(g_LogSigEmitter && g_LogWorker)
    {
        g_LogWorker->moveToThread(&th);
        QObject::connect(&th, &QThread::finished, g_LogWorker, &QObject::deleteLater);
        QObject::connect(g_LogSigEmitter, &LogSigEmitter::record_log,
                         g_LogWorker , &Logger::receive_log,
                         Qt::QueuedConnection);
        th.start();
        return true;
    }
    else
    {
        return false;
    }
}
void end_log_thread(QThread &th)
{
    /*
     * emitter is managed by main thread,
     * and worker is managed by log thread (th).
    */
    if(g_LogSigEmitter && g_LogWorker)
    {
        QObject::disconnect(g_LogSigEmitter, &LogSigEmitter::record_log,
                         g_LogWorker , &Logger::receive_log);
        th.quit();
        th.wait();
        g_LogWorker = nullptr;
        delete g_LogSigEmitter; g_LogSigEmitter = nullptr;
    }
    if(g_LogSigEmitter)
    {
        delete g_LogSigEmitter; g_LogSigEmitter = nullptr;
    }
}
