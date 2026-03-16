#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QObject>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>

class SettingsManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString outputDir READ outputDir WRITE setOutputDir NOTIFY outputDirChanged)
    Q_PROPERTY(bool openFolderWhenFinished READ openFolderWhenFinished WRITE setOpenFolderWhenFinished NOTIFY openFolderWhenFinishedChanged)
    // Сюда можно добавить любые другие глобальные настройки (тема, звук по завершению и т.д.)

public:
    explicit SettingsManager(QObject *parent = nullptr) : QObject(parent) {
        QSettings settings;

        // Получаем системную папку "Видео" пользователя по умолчанию
        QString defaultVideoPath = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
        if (defaultVideoPath.isEmpty()) {
            defaultVideoPath = QDir::homePath(); // Фолбэк
        }

        // Читаем из реестра/конфига, если нет - ставим дефолтную
        m_outputDir = settings.value("Global/outputDir", defaultVideoPath).toString();
        m_openFolderWhenFinished = settings.value("Global/openFolderWhenFinished", true).toBool();
    }

    QString outputDir() const { return m_outputDir; }
    bool openFolderWhenFinished() const { return m_openFolderWhenFinished; }

public slots:
    void setOutputDir(const QString& dir) {
        if (m_outputDir == dir) return;
        m_outputDir = dir;
        QSettings().setValue("Global/outputDir", dir);
        emit outputDirChanged();
    }

    void setOpenFolderWhenFinished(bool val) {
        if (m_openFolderWhenFinished == val) return;
        m_openFolderWhenFinished = val;
        QSettings().setValue("Global/openFolderWhenFinished", val);
        emit openFolderWhenFinishedChanged();
    }

signals:
    void outputDirChanged();
    void openFolderWhenFinishedChanged();

private:
    QString m_outputDir;
    bool m_openFolderWhenFinished;
};

#endif // SETTINGSMANAGER_H
