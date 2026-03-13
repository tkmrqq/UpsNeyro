#ifndef FILTERMANAGER_H
#define FILTERMANAGER_H

#include <QObject>
#include <QDebug>

class FilterManager : public QObject {

    Q_OBJECT

    Q_PROPERTY(int brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)
    Q_PROPERTY(int contrast READ contrast WRITE setContrast NOTIFY contrastChanged)
    Q_PROPERTY(int saturation READ saturation WRITE setSaturation NOTIFY saturationChanged)
    Q_PROPERTY(int blur READ blur WRITE setBlur NOTIFY blurChanged)
    Q_PROPERTY(FilterPreset activePreset READ activePreset NOTIFY activePresetChanged)

public:

    enum FilterPreset {
        PresetNone = 0,
        PresetCinematic,
        PresetVibrant,
        PresetBW,
        PresetVintage
    };

    Q_ENUM(FilterPreset) //visible for QML

    explicit FilterManager(QObject *parent = nullptr) : QObject(parent) {}

    int brightness() const { return m_brightness; }
    int contrast() const { return m_contrast; }
    int saturation() const { return m_saturation; }
    int blur() const { return m_blur; }
    FilterPreset activePreset() const { return m_activePreset; }

    Q_INVOKABLE void resetAll() {
        setBrightness(0);
        setContrast(0);
        setSaturation(100);
        setBlur(0);
        qDebug() << "Filters reset to default!";
        m_activePreset = PresetNone;
        emit activePresetChanged();
    }

    Q_INVOKABLE void applyPreset(FilterPreset preset) {
        m_activePreset = preset;
        emit activePresetChanged();
        switch (preset) {
        case PresetCinematic:
            setBrightness(-10); setContrast(20); setSaturation(80); setBlur(0); break;
        case PresetVibrant:
            setBrightness(5); setContrast(10); setSaturation(150); setBlur(0); break;
        case PresetBW:
            setBrightness(0); setContrast(30); setSaturation(0); setBlur(0); break;
        case PresetVintage:
            setBrightness(10); setContrast(20); setSaturation(75); setBlur(5); break;
        default: resetAll(); break;
        }
        qDebug() << "Applied preset:" << preset;
    }

public slots:
    void setBrightness(int val) { if (m_brightness == val) return; m_brightness = val; emit brightnessChanged(); }
    void setContrast(int val) { if (m_contrast == val) return; m_contrast = val; emit contrastChanged(); }
    void setSaturation(int val) { if (m_saturation == val) return; m_saturation = val; emit saturationChanged(); }
    void setBlur(int val) { if (m_blur == val) return; m_blur = val; emit blurChanged(); }

signals:
    void activePresetChanged();
    void brightnessChanged();
    void contrastChanged();
    void saturationChanged();
    void blurChanged();

private:
    FilterPreset m_activePreset = PresetNone;
    int m_brightness = 0;
    int m_contrast = 0;
    int m_saturation = 100;
    int m_blur = 0;
};

#endif // FILTERMANAGER_H
