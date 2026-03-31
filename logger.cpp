#include "logger.h"
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QDebug>

Logger* Logger::instance()
{
    static Logger inst;
    return &inst;
}

Logger::Logger(QObject *parent) : QObject(parent)
{
    const QString dir = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);

    m_file.setFileName(dir + "/upsneyro.log");
    m_file.open(QIODevice::Append | QIODevice::Text);
    m_stream.setDevice(&m_file);

    info("=== Session started ===");
}

void Logger::log(Level level, const QString &message)
{
    const QString ts = QDateTime::currentDateTime()
                           .toString("yyyy-MM-dd HH:mm:ss");
    const QString lvl = (level == Debug)   ? "DEBUG"   :
                            (level == Info)    ? "INFO"    :
                            (level == Warning) ? "WARNING" : "ERROR";

    const QString line = QString("[%1] [%2] %3").arg(ts, lvl, message);

    m_stream << line << "\n";
    m_stream.flush();
    qDebug().noquote() << line;
}

QString Logger::logFilePath() const
{
    return m_file.fileName();
}
