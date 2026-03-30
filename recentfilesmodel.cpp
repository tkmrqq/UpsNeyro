#include "recentfilesmodel.h"
#include <QSettings>

// RecentFilesModel::RecentFilesModel(QObject *parent)
//     : QObject(parent)
// {
//     load();
// }

void RecentFilesModel::addFile(const QString &path)
{
    if (path.isEmpty()) return;
    m_files.removeAll(path);          // убрать дубль
    m_files.prepend(path);            // добавить в начало
    while (m_files.size() > MaxFiles)
        m_files.removeLast();
    save();
    emit filesChanged();
}

void RecentFilesModel::removeFile(const QString &path)
{
    if (m_files.removeAll(path) > 0) {
        save();
        emit filesChanged();
    }
}

void RecentFilesModel::clear()
{
    if (m_files.isEmpty()) return;
    m_files.clear();
    save();
    emit filesChanged();
}

void RecentFilesModel::load()
{
    QSettings s;
    m_files = s.value("recentFiles").toStringList();
}

void RecentFilesModel::save()
{
    QSettings s;
    s.setValue("recentFiles", m_files);
}
