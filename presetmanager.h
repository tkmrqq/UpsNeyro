#pragma once
#include <QObject>
#include <QList>
#include <QJsonObject>
#include <QtQml/qqmlregistration.h>
#include "framefilter.h" // ← FilterParams
#include "filtermanager.h"

struct FilterPreset
{
    QString name;
    bool isBuiltin = false;
    FilterParams params;

    QJsonObject toJson() const;
    static FilterPreset fromJson(const QJsonObject &obj);
};

class PresetManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QStringList presetNames READ presetNames NOTIFY presetsChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)

public:
    explicit PresetManager(QObject *parent = nullptr);

    QStringList presetNames() const;
    int currentIndex() const { return m_currentIndex; }

    // Возвращает FilterParams для применения к FrameFilter
    Q_INVOKABLE FilterParams paramsAt(int index) const;
    Q_INVOKABLE FilterParams currentParams() const;
    Q_INVOKABLE QString nameAt(int index) const;
    Q_INVOKABLE bool isBuiltin(int index) const;

    // CRUD
    Q_INVOKABLE void savePreset(const QString &name, const FilterParams &params);
    Q_INVOKABLE void deletePreset(int index);
    Q_INVOKABLE void renamePreset(int index, const QString &newName);

    Q_INVOKABLE void savePresetFromValues(const QString &name,
                                          int brightness, int contrast, int saturation, int hue,
                                          int sharpness, int blur, int vignette, int grain);

    Q_INVOKABLE void applyPresetTo(int index, FilterManager *fm) const;

    void setCurrentIndex(int i);

signals:
    void presetsChanged();
    void currentIndexChanged();

private:
    void buildBuiltins();
    void loadFromDisk();
    void saveToDisk() const;
    QString presetsFilePath() const;

    QList<FilterPreset> m_presets;
    int m_currentIndex = 0;
};
