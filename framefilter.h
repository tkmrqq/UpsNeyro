#pragma once

#include <QObject>
#include <QString>
#include <cstdint>

// Параметры фильтров — все значения нормализованы
struct FilterParams
{
    // ── Стандартные параметры (уже есть) ─────────────────────────────────────
    float brightness = 1.0f; // [0.0 .. 2.0], нейтрально = 1.0
    float contrast = 1.0f;   // [0.0 .. 2.0], нейтрально = 1.0
    float saturation = 1.0f; // [0.0 .. 2.0], нейтрально = 1.0
    float hue = 0.0f;        // [-180 .. 180] градусы
    float sharpness = 0.0f;  // [0.0 .. 1.0]
    float blur = 0.0f;       // [0.0 .. 1.0]
    float vignette = 0.0f;   // [0.0 .. 1.0]
    float grain = 0.0f;      // [0.0 .. 1.0]

    // ── Kernel-based эффекты ──────────────────────────────────────────────────
    enum KernelType
    {
        KernelNone = 0,
        KernelPrewitt, // Edge detection (Prewitt operator)
        KernelEmboss,  // Emboss effect
        KernelMinMax   // Local contrast stretch
    };
    KernelType kernelType = KernelNone;

    bool isIdentity() const
    {
        return qFuzzyCompare(brightness, 1.0f) && qFuzzyCompare(contrast, 1.0f) && qFuzzyCompare(saturation, 1.0f) && qFuzzyIsNull(hue) && qFuzzyIsNull(sharpness) && qFuzzyIsNull(blur) && qFuzzyIsNull(vignette) && qFuzzyIsNull(grain) && kernelType == KernelNone;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// FrameFilter — применяет фильтры к RGB24 кадру
// Автоматически выбирает CUDA или CPU бэкенд
// ─────────────────────────────────────────────────────────────────────────────
class FrameFilter
{
public:
    FrameFilter();
    ~FrameFilter();

    // Инициализация — определяет доступный бэкенд
    // Возвращает true если CUDA доступна
    bool init();

    bool isCudaAvailable() const { return m_cudaAvailable; }
    QString backendName() const { return m_cudaAvailable ? "CUDA" : "CPU"; }

    // Применить фильтры к кадру на месте (in-place)
    // data — RGB24 буфер, width*height*3 байт
    void apply(uint8_t *data, int width, int height,
               const FilterParams &params);

private:
    // CPU реализации
    void applyBrightness(uint8_t *data, int size, float value);
    void applyContrast(uint8_t *data, int size, float value);
    void applySaturation(uint8_t *data, int pixels, float value);
    void applyHue(uint8_t *data, int pixels, float degrees);
    void applyBlur(uint8_t *data, int w, int h, float strength);
    void applySharpness(uint8_t *data, int w, int h, float strength);
    void applyVignette(uint8_t *data, int w, int h, float strength);
    void applyGrain(uint8_t *data, int size, float strength);
    void applyKernel(uint8_t *data, int w, int h, FilterParams::KernelType type);

#ifdef USE_CUDA
    // CUDA реализации — объявления, тела в .cu файле
    void applyAllCuda(uint8_t *data, int width, int height,
                      const FilterParams &params);
    void *m_cudaBuffer = nullptr; // GPU буфер
    int m_cudaBufSize = 0;
#endif

    bool m_cudaAvailable = false;
    bool m_initialized = false;
};
