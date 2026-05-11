#pragma once

#include <QString>
#include <QtGlobal>

/** Long edge (px) implied by UI preset names used in UpscalePage / UpscaleManager. */
inline int targetLongEdgeForPreset(const QString &preset)
{
    if (preset == QLatin1String("1080p"))
        return 1920;
    if (preset == QLatin1String("2K"))
        return 2560;
    if (preset == QLatin1String("4K"))
        return 3840;
    if (preset == QLatin1String("8K"))
        return 7680;
    return 3840;
}

/**
 * RealESRGAN path supports 2x or 4x per pass. Pick the smallest factor in {2,4}
 * so the longer side reaches at least targetLongEdgeForPreset (when possible).
 */
inline int upscaleScaleForTargetPreset(const QString &preset, int srcW, int srcH)
{
    if (srcW <= 0 || srcH <= 0)
        return 4;
    const int ls = qMax(srcW, srcH);
    const int targetLong = targetLongEdgeForPreset(preset);
    if (ls * 2 >= targetLong)
        return 2;
    return 4;
}
