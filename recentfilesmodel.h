#ifndef RECENTFILESMODEL_H
#define RECENTFILESMODEL_H

#include <QObject>
#include <QStringList>

class RecentFilesModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList files READ files NOTIFY filesChanged)

public:
    explicit RecentFilesModel(QObject *parent = nullptr);

    QStringList files() const { return m_files; }

    Q_INVOKABLE void addFile(const QString &path);
    Q_INVOKABLE void removeFile(const QString &path);
    Q_INVOKABLE void clear();

signals:
    void filesChanged();

private:
    void load();
    void save();

    QStringList m_files;
    static constexpr int MaxFiles = 10;
};

#endif
