#ifndef FILTERMANAGER_H
#define FILTERMANAGER_H
#pragma once

#include <QObject>
#include <QMap>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include "framefilter.h"

class FilterManager : public QObject {
    Q_OBJECT
    QML_ELEMENT

    // Диапазоны для QML: int-friendly, маппинг внутри сеттеров
    // brightness: QML [-100..100] → C++ [0.0..2.0], нейтраль = 0 → 1.0f
    // contrast:   QML [-100..100] → C++ [0.0..2.0], нейтраль = 0 → 1.0f
    // saturation: QML [0..200]    → C++ [0.0..2.0], нейтраль = 100 → 1.0f
    // hue:        QML [-180..180] → C++ [-180..180], без маппинга
    // остальные:  QML [0..100]    → C++ [0.0..1.0]

    Q_PROPERTY(int brightness  READ brightnessQml  WRITE setBrightnessQml  NOTIFY paramsChanged)
    Q_PROPERTY(int contrast    READ contrastQml    WRITE setContrastQml    NOTIFY paramsChanged)
    Q_PROPERTY(int saturation  READ saturationQml  WRITE setSaturationQml  NOTIFY paramsChanged)
    Q_PROPERTY(int hue         READ hueQml         WRITE setHueQml         NOTIFY paramsChanged)
    Q_PROPERTY(int sharpness   READ sharpnessQml   WRITE setSharpnessQml   NOTIFY paramsChanged)
    Q_PROPERTY(int blur        READ blurQml        WRITE setBlurQml        NOTIFY paramsChanged)
    Q_PROPERTY(int vignette    READ vignetteQml    WRITE setVignetteQml    NOTIFY paramsChanged)
    Q_PROPERTY(int grain       READ grainQml       WRITE setGrainQml       NOTIFY paramsChanged)

    Q_PROPERTY(PresetType activePreset READ activePreset NOTIFY presetChanged)
    Q_PROPERTY(bool hasKernelPreset   READ hasKernelPreset NOTIFY presetChanged)

public:
    // ── Enum пресетов для QML ─────────────────────────────────────────────────
    enum PresetType {
        PresetNone     = -1,
        PresetCinematic = 0,
        PresetVibrant,
        PresetBW,
        PresetVintage,
        PresetPrewitt,
        PresetEmboss,
        PresetMinMax
    };
    Q_ENUM(PresetType)

    explicit FilterManager(QObject *parent = nullptr) : QObject(parent) {
        buildPresets();
    }

    // ── QML геттеры (int диапазоны) ───────────────────────────────────────────
    int brightnessQml()  const { return qRound((m_params.brightness - 1.0f) * 100.0f); } // [-100..100]
    int contrastQml()    const { return qRound((m_params.contrast   - 1.0f) * 100.0f); } // [-100..100]
    int saturationQml()  const { return qRound(m_params.saturation * 100.0f);           } // [0..200]
    int hueQml()         const { return qRound(m_params.hue);                            } // [-180..180]
    int sharpnessQml()   const { return qRound(m_params.sharpness * 100.0f);             } // [0..100]
    int blurQml()        const { return qRound(m_params.blur       * 100.0f);             } // [0..100]
    int vignetteQml()    const { return qRound(m_params.vignette   * 100.0f);             } // [0..100]
    int grainQml()       const { return qRound(m_params.grain      * 100.0f);             } // [0..100]

    PresetType activePreset()  const { return m_activePreset; }
    bool       hasKernelPreset() const {
        return m_activePreset == PresetPrewitt
               || m_activePreset == PresetEmboss
               || m_activePreset == PresetMinMax;
    }

    // ── QML сеттеры (маппинг → FilterParams) ─────────────────────────────────
    void setBrightnessQml(int v) { m_params.brightness = 1.0f + v / 100.0f; clearPreset(); emit paramsChanged(); }
    void setContrastQml  (int v) { m_params.contrast   = 1.0f + v / 100.0f; clearPreset(); emit paramsChanged(); }
    void setSaturationQml(int v) { m_params.saturation = v / 100.0f;         clearPreset(); emit paramsChanged(); }
    void setHueQml       (int v) { m_params.hue        = float(v);           clearPreset(); emit paramsChanged(); }
    void setSharpnessQml (int v) { m_params.sharpness  = v / 100.0f;         clearPreset(); emit paramsChanged(); }
    void setBlurQml      (int v) { m_params.blur        = v / 100.0f;         clearPreset(); emit paramsChanged(); }
    void setVignetteQml  (int v) { m_params.vignette   = v / 100.0f;         clearPreset(); emit paramsChanged(); }
    void setGrainQml     (int v) { m_params.grain       = v / 100.0f;         clearPreset(); emit paramsChanged(); }

    // ── Для C++ (UpscaleManager читает это) ───────────────────────────────────
    FilterParams currentParams() const { return m_params; }
    void setParams(const FilterParams &p) {
        m_params = p;
        m_activePreset = PresetNone;
        emit paramsChanged();
        emit presetChanged();
    }

    // ── QML API ───────────────────────────────────────────────────────────────
    Q_INVOKABLE void applyPreset(PresetType preset) {
        if (!m_presets.contains(preset)) return;
        m_params       = m_presets[preset];
        m_activePreset = preset;
        emit paramsChanged();
        emit presetChanged();
    }

    Q_INVOKABLE void resetAll() {
        m_params = FilterParams{};
        clearPreset();
        emit paramsChanged();
    }

signals:
    void paramsChanged();
    void presetChanged();

private:
    FilterParams              m_params;
    PresetType                m_activePreset = PresetNone;
    QMap<PresetType, FilterParams> m_presets;

    void clearPreset() {
        if (m_activePreset != PresetNone) {
            m_activePreset = PresetNone;
            emit presetChanged();
        }
    }

    void buildPresets() {
        FilterParams cinematic;
        cinematic.brightness = 0.9f;
        cinematic.contrast   = 1.2f;
        cinematic.saturation = 0.85f;
        cinematic.sharpness  = 0.3f;
        cinematic.vignette   = 0.4f;
        m_presets[PresetCinematic] = cinematic;

        FilterParams vibrant;
        vibrant.brightness = 1.05f;
        vibrant.contrast   = 1.15f;
        vibrant.saturation = 1.6f;
        vibrant.sharpness  = 0.4f;
        m_presets[PresetVibrant] = vibrant;

        FilterParams bw;
        bw.saturation = 0.0f;
        bw.contrast   = 1.1f;
        bw.sharpness  = 0.2f;
        m_presets[PresetBW] = bw;

        FilterParams vintage;
        vintage.brightness = 0.95f;
        vintage.contrast   = 0.9f;
        vintage.saturation = 0.75f;
        vintage.hue        = 8.0f;
        vintage.grain      = 0.35f;
        vintage.vignette   = 0.5f;
        m_presets[PresetVintage] = vintage;

        FilterParams prewitt;
        prewitt.kernelType = FilterParams::KernelPrewitt;
        m_presets[PresetPrewitt] = prewitt;

        FilterParams emboss;
        emboss.kernelType = FilterParams::KernelEmboss;
        m_presets[PresetEmboss] = emboss;

        FilterParams minmax;
        minmax.kernelType = FilterParams::KernelMinMax;
        m_presets[PresetMinMax] = minmax;
    }
};

#endif // FILTERMANAGER_H
