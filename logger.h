#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QFile>
#include <QTextStream>

class Logger : public QObject
{
    Q_OBJECT
    // QML_ELEMENT

public:
    enum Level { Debug, Info, Warning, Error };
    Q_ENUM(Level)

    static Logger* instance();

    Q_INVOKABLE void log(Level level, const QString &message);
    Q_INVOKABLE void debug(const QString &msg)   { log(Debug,   msg); }
    Q_INVOKABLE void info(const QString &msg)    { log(Info,    msg); }
    Q_INVOKABLE void warning(const QString &msg) { log(Warning, msg); }
    Q_INVOKABLE void error(const QString &msg)   { log(Error,   msg); }

    QString logFilePath() const;

private:
    explicit Logger(QObject *parent = nullptr);
    QFile       m_file;
    QTextStream m_stream;
};

#endif
