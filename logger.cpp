#include "logger.h"
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>

Logger::Logger(QObject *parent) : QObject(parent)
{

}

/**
 * @brief Logger::instance 获得单例对象
 * @return
 */
Logger *Logger::instance()
{
    static Logger logger;
    return &logger;
}

/**
 * @brief Logger::writeLog 写入日志
 * @param fileName 产生日志的文件名
 * @param lineNo 产生日志的行号
 * @param level 日志级别
 * @param log 日志内容
 */
void Logger::writeLog(QString fileName, int lineNo, LOG_LEVEL level, QString msg)
{
    static const char *levels[] = {"DEBUG", "INFO", "WARN", "ERROR"};
    static const char* log_dir_str = "log", *log_file_str = "log";
    QFileInfo info(log_dir_str);
    QString path = info.absoluteFilePath();
    QDir dir(path);
    if(!dir.exists())
        dir.mkpath(path);
    QString date=QDateTime::currentDateTime().toString("yyyy-MM-dd");
    QString time=QDateTime::currentDateTime().toString("hh:mm:ss");
    QFile file(QString(log_dir_str) + QString("/")
               + QString(log_file_str) + QString("_%1.txt").arg(date));
    if(!file.open(QFile::WriteOnly | QFile::Append))
        return;
    QTextStream in(&file);
    in<<date<<" "<<time<<"  "<<fileName<<"  ["<<lineNo<<"]"<<"  ["<<levels[level]<<"]\r\n";
    in<<msg<<"\r\n";
    file.flush();
    file.close();
}
