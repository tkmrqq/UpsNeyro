#include "framefilter.h"

#include <QDebug>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <random>

// CUDA доступна только если собрано с USE_CUDA
#ifdef USE_CUDA
#include "framefilter_cuda.cuh"
#include "cuda_runtime.h"
#endif

// ─────────────────────────────────────────────────────────────────────────────
// Вспомогательные функции
// ─────────────────────────────────────────────────────────────────────────────

static inline uint8_t clamp8(float v) {
    return static_cast<uint8_t>(std::max(0.0f, std::min(255.0f, v)));
}

// RGB → HSV
static void rgbToHsv(float r, float g, float b,
                     float &h, float &s, float &v)
{
    float max = std::max({r, g, b});
    float min = std::min({r, g, b});
    float delta = max - min;

    v = max;
    s = (max > 0.0f) ? delta / max : 0.0f;

    if (delta < 1e-6f) { h = 0.0f; return; }

    if      (max == r) h = 60.0f * std::fmod((g - b) / delta, 6.0f);
    else if (max == g) h = 60.0f * ((b - r) / delta + 2.0f);
    else               h = 60.0f * ((r - g) / delta + 4.0f);

    if (h < 0.0f) h += 360.0f;
}

// HSV → RGB
static void hsvToRgb(float h, float s, float v,
                     float &r, float &g, float &b)
{
    if (s < 1e-6f) { r = g = b = v; return; }

    float hh = h / 60.0f;
    int   i  = static_cast<int>(hh);
    float f  = hh - i;
    float p  = v * (1.0f - s);
    float q  = v * (1.0f - s * f);
    float t  = v * (1.0f - s * (1.0f - f));

    switch (i % 6) {
    case 0: r=v; g=t; b=p; break;
    case 1: r=q; g=v; b=p; break;
    case 2: r=p; g=v; b=t; break;
    case 3: r=p; g=q; b=v; break;
    case 4: r=t; g=p; b=v; break;
    default:r=v; g=p; b=q; break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / init
// ─────────────────────────────────────────────────────────────────────────────

FrameFilter::FrameFilter() {}

FrameFilter::~FrameFilter()
{
#ifdef USE_CUDA
    if (m_cudaBuffer) {
        cudaFree(m_cudaBuffer);
        m_cudaBuffer = nullptr;
    }
#endif
}

bool FrameFilter::init()
{
    if (m_initialized) return m_cudaAvailable;

#ifdef USE_CUDA
    int deviceCount = 0;
    cudaError_t err = cudaGetDeviceCount(&deviceCount);
    if (err == cudaSuccess && deviceCount > 0) {
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, 0);
        qDebug() << "[FrameFilter] CUDA available:" << prop.name;
        m_cudaAvailable = true;
    } else {
        qDebug() << "[FrameFilter] CUDA not available, using CPU";
        m_cudaAvailable = false;
    }
#else
    qDebug() << "[FrameFilter] Built without CUDA, using CPU";
    m_cudaAvailable = false;
#endif

    m_initialized = true;
    return m_cudaAvailable;
}

// ─────────────────────────────────────────────────────────────────────────────
// apply() — точка входа
// ─────────────────────────────────────────────────────────────────────────────

void FrameFilter::apply(uint8_t *data, int width, int height, const FilterParams &params)
{
    if (params.isIdentity()) return;

    const int pixels = width * height;
    const int size   = pixels * 3;

#ifdef USE_CUDA
    if (m_cudaAvailable) {
        applyFiltersCuda(data, width, height, params);
        return;
    }
#endif

    // CPU path — исправленные условия
    if (!qFuzzyCompare(params.brightness, 1.0f)) applyBrightness(data, size, params.brightness);
    if (!qFuzzyCompare(params.contrast,   1.0f)) applyContrast  (data, size, params.contrast);
    if (!qFuzzyCompare(params.saturation, 1.0f)) applySaturation(data, pixels, params.saturation);
    if (!qFuzzyIsNull (params.hue))              applyHue       (data, pixels, params.hue);
    if (params.blur      > 0.0f)                 applyBlur      (data, width, height, params.blur);
    if (params.sharpness > 0.0f)                 applySharpness (data, width, height, params.sharpness);
    if (params.vignette  > 0.0f)                 applyVignette  (data, width, height, params.vignette);
    if (params.grain     > 0.0f)                 applyGrain     (data, size, params.grain);

    // Kernel-based пресеты (только CPU; CUDA-путь обрабатывает их в applyFiltersCuda)
    if (params.kernelType != FilterParams::KernelNone) {
        applyKernel(data, width, height, params.kernelType);
    }
}

void FrameFilter::applyKernel(uint8_t *data, int w, int h, FilterParams::KernelType type)
{
    // Работаем только с Y (luma) через условный RGB→grey для edge-эффектов
    std::vector<uint8_t> src(data, data + w * h * 3);

    auto px = [&](int y, int x, int ch) -> int {
        y = std::clamp(y, 0, h - 1);
        x = std::clamp(x, 0, w - 1);
        return src[(y * w + x) * 3 + ch];
    };

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            for (int c = 0; c < 3; ++c) {
                int result = 0;
                if (type == FilterParams::KernelPrewitt) {
                    int gx = -px(y-1,x-1,c) + px(y-1,x+1,c)
                             -px(y,  x-1,c) + px(y,  x+1,c)
                             -px(y+1,x-1,c) + px(y+1,x+1,c);
                    int gy = -px(y-1,x-1,c) - px(y-1,x,c) - px(y-1,x+1,c)
                             +px(y+1,x-1,c) + px(y+1,x,c) + px(y+1,x+1,c);
                    result = std::min(255, (int)std::sqrt(float(gx*gx + gy*gy)));
                }
                else if (type == FilterParams::KernelEmboss) {
                    result = std::clamp(
                        -2*px(y-1,x-1,c) - px(y-1,x,c)
                            - px(y,  x-1,c) + px(y,  x,c) + px(y,  x+1,c)
                            + px(y+1,x,  c) + 2*px(y+1,x+1,c) + 128,
                        0, 255);
                }
                else if (type == FilterParams::KernelMinMax) {
                    uint8_t mn = 255, mx = 0;
                    for (int dy = -1; dy <= 1; ++dy)
                        for (int dx = -1; dx <= 1; ++dx) {
                            uint8_t v = (uint8_t)px(y+dy, x+dx, c);
                            mn = std::min(mn, v); mx = std::max(mx, v);
                        }
                    float range = mx - mn;
                    result = range > 0
                                 ? (int)(((px(y,x,c) - mn) / range) * 255.0f)
                                 : px(y,x,c);
                }
                data[(y * w + x) * 3 + c] = (uint8_t)result;
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// CPU реализации
// ─────────────────────────────────────────────────────────────────────────────

void FrameFilter::applyBrightness(uint8_t *data, int size, float value)
{
    // value: -1..+1 → -255..+255 добавка к пикселю
    const float add = value * 255.0f;
    for (int i = 0; i < size; ++i)
        data[i] = clamp8(data[i] + add);
}

void FrameFilter::applyContrast(uint8_t *data, int size, float value)
{
    // value: 0..4, 1 = нет изменений
    // формула: out = (in - 128) * contrast + 128
    for (int i = 0; i < size; ++i)
        data[i] = clamp8((data[i] - 128.0f) * value + 128.0f);
}

void FrameFilter::applySaturation(uint8_t *data, int pixels, float value)
{
    // Через HSV: умножаем S на value
    for (int i = 0; i < pixels; ++i) {
        float r = data[i*3+0] / 255.0f;
        float g = data[i*3+1] / 255.0f;
        float b = data[i*3+2] / 255.0f;

        float h, s, v;
        rgbToHsv(r, g, b, h, s, v);
        s = std::min(1.0f, s * value);
        hsvToRgb(h, s, v, r, g, b);

        data[i*3+0] = clamp8(r * 255.0f);
        data[i*3+1] = clamp8(g * 255.0f);
        data[i*3+2] = clamp8(b * 255.0f);
    }
}

void FrameFilter::applyHue(uint8_t *data, int pixels, float degrees)
{
    for (int i = 0; i < pixels; ++i) {
        float r = data[i*3+0] / 255.0f;
        float g = data[i*3+1] / 255.0f;
        float b = data[i*3+2] / 255.0f;

        float h, s, v;
        rgbToHsv(r, g, b, h, s, v);
        h = std::fmod(h + degrees + 360.0f, 360.0f);
        hsvToRgb(h, s, v, r, g, b);

        data[i*3+0] = clamp8(r * 255.0f);
        data[i*3+1] = clamp8(g * 255.0f);
        data[i*3+2] = clamp8(b * 255.0f);
    }
}

void FrameFilter::applyBlur(uint8_t *data, int w, int h, float strength)
{
    // Box blur — радиус зависит от strength (0..1 → 1..8 пикселей)
    const int radius = static_cast<int>(1 + strength * 7);

    std::vector<uint8_t> tmp(w * h * 3);
    std::memcpy(tmp.data(), data, w * h * 3);

    // Горизонтальный проход
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float r = 0, g = 0, b = 0;
            int count = 0;
            for (int dx = -radius; dx <= radius; ++dx) {
                int nx = std::max(0, std::min(w - 1, x + dx));
                r += tmp[(y*w + nx)*3 + 0];
                g += tmp[(y*w + nx)*3 + 1];
                b += tmp[(y*w + nx)*3 + 2];
                ++count;
            }
            data[(y*w + x)*3 + 0] = clamp8(r / count);
            data[(y*w + x)*3 + 1] = clamp8(g / count);
            data[(y*w + x)*3 + 2] = clamp8(b / count);
        }
    }

    std::memcpy(tmp.data(), data, w * h * 3);

    // Вертикальный проход
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float r = 0, g = 0, b = 0;
            int count = 0;
            for (int dy = -radius; dy <= radius; ++dy) {
                int ny = std::max(0, std::min(h - 1, y + dy));
                r += tmp[(ny*w + x)*3 + 0];
                g += tmp[(ny*w + x)*3 + 1];
                b += tmp[(ny*w + x)*3 + 2];
                ++count;
            }
            data[(y*w + x)*3 + 0] = clamp8(r / count);
            data[(y*w + x)*3 + 1] = clamp8(g / count);
            data[(y*w + x)*3 + 2] = clamp8(b / count);
        }
    }
}

void FrameFilter::applySharpness(uint8_t *data, int w, int h, float strength)
{
    // Unsharp mask: out = original + strength * (original - blurred)
    std::vector<uint8_t> blurred(w * h * 3);
    std::memcpy(blurred.data(), data, w * h * 3);

    // Лёгкий блюр для маски
    FrameFilter tmp;
    tmp.applyBlur(blurred.data(), w, h, 0.1f);

    for (int i = 0; i < w * h * 3; ++i) {
        float orig  = data[i];
        float diff  = orig - blurred[i];
        data[i] = clamp8(orig + strength * diff * 2.0f);
    }
}

void FrameFilter::applyVignette(uint8_t *data, int w, int h, float strength)
{
    const float cx = w * 0.5f;
    const float cy = h * 0.5f;
    const float maxDist = std::sqrt(cx * cx + cy * cy);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float dx   = (x - cx) / maxDist;
            float dy   = (y - cy) / maxDist;
            float dist = std::sqrt(dx*dx + dy*dy);
            // Виньетка: 1 в центре, убывает к краям
            float factor = 1.0f - strength * dist * dist * 1.5f;
            factor = std::max(0.0f, factor);

            int idx = (y * w + x) * 3;
            data[idx+0] = clamp8(data[idx+0] * factor);
            data[idx+1] = clamp8(data[idx+1] * factor);
            data[idx+2] = clamp8(data[idx+2] * factor);
        }
    }
}

void FrameFilter::applyGrain(uint8_t *data, int size, float strength)
{
    static std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    const float amplitude = strength * 30.0f;
    for (int i = 0; i < size; ++i)
        data[i] = clamp8(data[i] + dist(rng) * amplitude);
}
