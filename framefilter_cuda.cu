// framefilter_cuda.cu — CUDA кернелы для фильтров

#include "framefilter_cuda.cuh"
#include <cuda_runtime.h>
#include <curand_kernel.h>
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
// Вспомогательные device функции
// ─────────────────────────────────────────────────────────────────────────────

__device__ inline float clampf(float v) {
    return fmaxf(0.0f, fminf(255.0f, v));
}

__device__ void rgbToHsv(float r, float g, float b,
                         float &h, float &s, float &v)
{
    float mx = fmaxf(fmaxf(r, g), b);
    float mn = fminf(fminf(r, g), b);
    float delta = mx - mn;

    v = mx;
    s = (mx > 0.0f) ? delta / mx : 0.0f;

    if (delta < 1e-6f) { h = 0.0f; return; }

    if      (mx == r) h = 60.0f * fmodf((g - b) / delta, 6.0f);
    else if (mx == g) h = 60.0f * ((b - r) / delta + 2.0f);
    else              h = 60.0f * ((r - g) / delta + 4.0f);

    if (h < 0.0f) h += 360.0f;
}

__device__ void hsvToRgb(float h, float s, float v,
                         float &r, float &g, float &b)
{
    if (s < 1e-6f) { r = g = b = v; return; }

    float hh = h / 60.0f;
    int   i  = (int)hh;
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
// Основной кернел — все фильтры за один проход
// ─────────────────────────────────────────────────────────────────────────────

__global__ void filterKernel(uint8_t *data, int width, int height,
                             float brightness, float contrast,
                             float saturation, float hue,
                             float grain, float vignetteStr)
{
    const int x = blockIdx.x * blockDim.x + threadIdx.x;
    const int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;

    const int idx = (y * width + x) * 3;

    float r = data[idx+0];
    float g = data[idx+1];
    float b = data[idx+2];

    // ── Brightness ────────────────────────────────────────────────────────────
    if (brightness != 0.0f) {
        float add = brightness * 255.0f;
        r += add; g += add; b += add;
    }

    // ── Contrast ──────────────────────────────────────────────────────────────
    if (contrast != 1.0f) {
        r = (r - 128.0f) * contrast + 128.0f;
        g = (g - 128.0f) * contrast + 128.0f;
        b = (b - 128.0f) * contrast + 128.0f;
    }

    // ── Saturation + Hue (через HSV) ──────────────────────────────────────────
    if (saturation != 1.0f || hue != 0.0f) {
        float h, s, v;
        rgbToHsv(r / 255.0f, g / 255.0f, b / 255.0f, h, s, v);

        if (saturation != 1.0f)
            s = fminf(1.0f, s * saturation);

        if (hue != 0.0f)
            h = fmodf(h + hue + 360.0f, 360.0f);

        float rf, gf, bf;
        hsvToRgb(h, s, v, rf, gf, bf);
        r = rf * 255.0f;
        g = gf * 255.0f;
        b = bf * 255.0f;
    }

    // ── Vignette ──────────────────────────────────────────────────────────────
    if (vignetteStr > 0.0f) {
        float cx = width  * 0.5f;
        float cy = height * 0.5f;
        float maxDist = sqrtf(cx*cx + cy*cy);
        float dx = (x - cx) / maxDist;
        float dy = (y - cy) / maxDist;
        float dist = sqrtf(dx*dx + dy*dy);
        float factor = fmaxf(0.0f, 1.0f - vignetteStr * dist * dist * 1.5f);
        r *= factor; g *= factor; b *= factor;
    }

    // ── Grain ─────────────────────────────────────────────────────────────────
    if (grain > 0.0f) {
        curandState state;
        curand_init((unsigned long long)(y * width + x), 0, 0, &state);
        float noise = (curand_uniform(&state) * 2.0f - 1.0f) * grain * 30.0f;
        r += noise; g += noise; b += noise;
    }

    data[idx+0] = (uint8_t)clampf(r);
    data[idx+1] = (uint8_t)clampf(g);
    data[idx+2] = (uint8_t)clampf(b);
}

// ─────────────────────────────────────────────────────────────────────────────
// Кернел размытия — два прохода (горизонтальный + вертикальный)
// ─────────────────────────────────────────────────────────────────────────────

__global__ void blurHKernel(const uint8_t *src, uint8_t *dst,
                            int width, int height, int radius)
{
    const int x = blockIdx.x * blockDim.x + threadIdx.x;
    const int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;

    float r=0, g=0, b=0;
    int count = 0;
    for (int dx = -radius; dx <= radius; ++dx) {
        int nx = max(0, min(width-1, x + dx));
        int idx = (y * width + nx) * 3;
        r += src[idx+0]; g += src[idx+1]; b += src[idx+2];
        ++count;
    }
    int out = (y * width + x) * 3;
    dst[out+0] = (uint8_t)(r / count);
    dst[out+1] = (uint8_t)(g / count);
    dst[out+2] = (uint8_t)(b / count);
}

__global__ void blurVKernel(const uint8_t *src, uint8_t *dst,
                            int width, int height, int radius)
{
    const int x = blockIdx.x * blockDim.x + threadIdx.x;
    const int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;

    float r=0, g=0, b=0;
    int count = 0;
    for (int dy = -radius; dy <= radius; ++dy) {
        int ny = max(0, min(height-1, y + dy));
        int idx = (ny * width + x) * 3;
        r += src[idx+0]; g += src[idx+1]; b += src[idx+2];
        ++count;
    }
    int out = (y * width + x) * 3;
    dst[out+0] = (uint8_t)(r / count);
    dst[out+1] = (uint8_t)(g / count);
    dst[out+2] = (uint8_t)(b / count);
}

// ─────────────────────────────────────────────────────────────────────────────
// Кернел резкости (unsharp mask)
// ─────────────────────────────────────────────────────────────────────────────

__global__ void sharpKernel(const uint8_t *orig, const uint8_t *blurred,
                            uint8_t *dst, int width, int height, float strength)
{
    const int x = blockIdx.x * blockDim.x + threadIdx.x;
    const int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;

    int idx = (y * width + x) * 3;
    for (int c = 0; c < 3; ++c) {
        float o = orig[idx+c];
        float d = o - blurred[idx+c];
        dst[idx+c] = (uint8_t)clampf(o + strength * d * 2.0f);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// C++ обёртка — вызывается из framefilter.cpp
// ─────────────────────────────────────────────────────────────────────────────

void applyFiltersCuda(uint8_t *hostData, int width, int height,
                      const FilterParams &p)
{
    const int size = width * height * 3;

    // Аллоцируем GPU буферы
    uint8_t *devData = nullptr;
    uint8_t *devTmp1 = nullptr;
    uint8_t *devTmp2 = nullptr;

    cudaMalloc(&devData, size);
    cudaMemcpy(devData, hostData, size, cudaMemcpyHostToDevice);

    dim3 block(16, 16);
    dim3 grid((width  + 15) / 16,
              (height + 15) / 16);

    // ── Blur ──────────────────────────────────────────────────────────────────
    if (p.blur > 0.0f) {
        const int radius = static_cast<int>(1 + p.blur * 7);
        cudaMalloc(&devTmp1, size);
        cudaMalloc(&devTmp2, size);

        blurHKernel<<<grid, block>>>(devData, devTmp1, width, height, radius);
        blurVKernel<<<grid, block>>>(devTmp1, devData, width, height, radius);

        cudaFree(devTmp1); devTmp1 = nullptr;
        cudaFree(devTmp2); devTmp2 = nullptr;
    }

    // ── Sharpness ─────────────────────────────────────────────────────────────
    if (p.sharpness > 0.0f) {
        cudaMalloc(&devTmp1, size);
        cudaMalloc(&devTmp2, size);

        // Лёгкий блюр для маски
        blurHKernel<<<grid, block>>>(devData, devTmp1, width, height, 1);
        blurVKernel<<<grid, block>>>(devTmp1, devTmp2, width, height, 1);

        sharpKernel<<<grid, block>>>(devData, devTmp2, devTmp1,
                                     width, height, p.sharpness);
        cudaMemcpy(devData, devTmp1, size, cudaMemcpyDeviceToDevice);

        cudaFree(devTmp1); devTmp1 = nullptr;
        cudaFree(devTmp2); devTmp2 = nullptr;
    }

    // ── Основные фильтры за один проход ───────────────────────────────────────
    filterKernel<<<grid, block>>>(
        devData, width, height,
        p.brightness, p.contrast,
        p.saturation, p.hue,
        p.grain, p.vignette
        );

    cudaMemcpy(hostData, devData, size, cudaMemcpyDeviceToHost);
    cudaFree(devData);
}
