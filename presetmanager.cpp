#include "presetmanager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>

// ── JSON сериализация FilterParams ───────────────────────────────────────────

static QJsonObject paramsToJson(const FilterParams &p)
{
    QJsonObject o;
    o["brightness"] = p.brightness;
    o["contrast"] = p.contrast;
    o["saturation"] = p.saturation;
    o["hue"] = p.hue;
    o["sharpness"] = p.sharpness;
    o["blur"] = p.blur;
    o["vignette"] = p.vignette;
    o["grain"] = p.grain;
    o["kernelType"] = static_cast<int>(p.kernelType);
    return o;
}

static FilterParams paramsFromJson(const QJsonObject &o)
{
    FilterParams p;
    p.brightness = o["brightness"].toDouble(1.0);
    p.contrast = o["contrast"].toDouble(1.0);
    p.saturation = o["saturation"].toDouble(1.0);
    p.hue = o["hue"].toDouble(0.0);
    p.sharpness = o["sharpness"].toDouble(0.0);
    p.blur = o["blur"].toDouble(0.0);
    p.vignette = o["vignette"].toDouble(0.0);
    p.grain = o["grain"].toDouble(0.0);
    p.kernelType = static_cast<FilterParams::KernelType>(o["kernelType"].toInt(0));
    return p;
}

QJsonObject FilterPreset::toJson() const
{
    QJsonObject o;
    o["name"] = name;
    o["params"] = paramsToJson(params);
    return o;
}

FilterPreset FilterPreset::fromJson(const QJsonObject &o)
{
    FilterPreset p;
    p.name = o["name"].toString();
    p.params = paramsFromJson(o["params"].toObject());
    return p;
}

// ── PresetManager ─────────────────────────────────────────────────────────────

PresetManager::PresetManager(QObject *parent) : QObject(parent)
{
    buildBuiltins();
    loadFromDisk();
}

void PresetManager::buildBuiltins()
{
    // Без фильтров — identity
    {
        FilterPreset p;
        p.name = tr("Без фильтров");
        p.isBuiltin = true;
        // params уже identity по умолчанию
        m_presets << p;
    }
    // Аниме
    {
        FilterPreset p;
        p.name = tr("Аниме");
        p.isBuiltin = true;
        p.params.sharpness = 0.6f;
        p.params.saturation = 1.15f;
        m_presets << p;
    }
    // Фильм
    {
        FilterPreset p;
        p.name = tr("Фильм");
        p.isBuiltin = true;
        p.params.sharpness = 0.4f;
        p.params.contrast = 1.1f;
        p.params.saturation = 0.95f;
        p.params.vignette = 0.2f;
        m_presets << p;
    }
    // Ретро
    {
        FilterPreset p;
        p.name = tr("Ретро");
        p.isBuiltin = true;
        p.params.saturation = 0.7f;
        p.params.contrast = 1.15f;
        p.params.grain = 0.15f;
        p.params.vignette = 0.3f;
        m_presets << p;
    }
}

QStringList PresetManager::presetNames() const
{
    QStringList names;
    names.reserve(m_presets.size());
    for (const auto &p : m_presets)
        names << p.name;
    return names;
}

FilterParams PresetManager::paramsAt(int index) const
{
    if (index < 0 || index >= m_presets.size())
        return FilterParams{};
    return m_presets.at(index).params;
}

FilterParams PresetManager::currentParams() const
{
    return paramsAt(m_currentIndex);
}

QString PresetManager::nameAt(int index) const
{
    if (index < 0 || index >= m_presets.size())
        return {};
    return m_presets.at(index).name;
}

bool PresetManager::isBuiltin(int index) const
{
    if (index < 0 || index >= m_presets.size())
        return false;
    return m_presets.at(index).isBuiltin;
}

void PresetManager::setCurrentIndex(int i)
{
    if (i == m_currentIndex || i < 0 || i >= m_presets.size())
        return;
    m_currentIndex = i;
    emit currentIndexChanged();
}

void PresetManager::savePreset(const QString &name, const FilterParams &params)
{
    // Найти существующий пользовательский с таким именем
    for (auto &p : m_presets)
    {
        if (!p.isBuiltin && p.name == name)
        {
            p.params = params;
            saveToDisk();
            emit presetsChanged();
            return;
        }
    }
    // Новый
    FilterPreset np;
    np.name = name;
    np.params = params;
    m_presets << np;
    saveToDisk();
    emit presetsChanged();
}

void PresetManager::deletePreset(int index)
{
    if (index < 0 || index >= m_presets.size())
        return;
    if (m_presets.at(index).isBuiltin)
        return; // встроенные нельзя удалить

    m_presets.removeAt(index);
    if (m_currentIndex >= m_presets.size())
        m_currentIndex = m_presets.size() - 1;

    saveToDisk();
    emit presetsChanged();
    emit currentIndexChanged();
}

void PresetManager::renamePreset(int index, const QString &newName)
{
    if (index < 0 || index >= m_presets.size())
        return;
    if (m_presets.at(index).isBuiltin)
        return;

    m_presets[index].name = newName;
    saveToDisk();
    emit presetsChanged();
}

QString PresetManager::presetsFilePath() const
{
    const QString dir = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/presets.json");
}

void PresetManager::saveToDisk() const
{
    QJsonArray arr;
    for (const auto &p : m_presets)
        if (!p.isBuiltin)
            arr.append(p.toJson());

    QFile f(presetsFilePath());
    if (f.open(QIODevice::WriteOnly))
        f.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
}

void PresetManager::loadFromDisk()
{
    QFile f(presetsFilePath());
    if (!f.open(QIODevice::ReadOnly))
        return;

    const auto arr = QJsonDocument::fromJson(f.readAll()).array();
    for (const auto &v : arr)
        m_presets << FilterPreset::fromJson(v.toObject());

    emit presetsChanged();
}

void PresetManager::savePresetFromValues(const QString &name,
                                         int brightness, int contrast, int saturation, int hue,
                                         int sharpness, int blur, int vignette, int grain)
{
    FilterParams p;
    // Конвертируем QML-диапазоны → FilterParams
    p.brightness = 1.0f + brightness / 100.0f;
    p.contrast = 1.0f + contrast / 100.0f;
    p.saturation = saturation / 100.0f;
    p.hue = float(hue);
    p.sharpness = sharpness / 100.0f;
    p.blur = blur / 100.0f;
    p.vignette = vignette / 100.0f;
    p.grain = grain / 100.0f;
    savePreset(name, p);
}

void PresetManager::applyPresetTo(int index, FilterManager *fm) const
{
    if (!fm || index < 0 || index >= m_presets.size())
        return;
    const FilterParams &p = m_presets.at(index).params;

    // Используем QML-сеттеры с теми же диапазонами что и ползунки
    fm->setBrightnessQml(qRound((p.brightness - 1.0f) * 100.0f)); // [-100..100]
    fm->setContrastQml(qRound((p.contrast - 1.0f) * 100.0f));     // [-100..100]
    fm->setSaturationQml(qRound(p.saturation * 100.0f));          // [0..200]
    fm->setHueQml(qRound(p.hue));                                 // [-180..180]
    fm->setSharpnessQml(qRound(p.sharpness * 100.0f));            // [0..100]
    fm->setBlurQml(qRound(p.blur * 100.0f));                      // [0..100]
    fm->setVignetteQml(qRound(p.vignette * 100.0f));              // [0..100]
    fm->setGrainQml(qRound(p.grain * 100.0f));                    // [0..100]
}
