#ifndef FRAMEFILTER_CUDA_CUH
#define FRAMEFILTER_CUDA_CUH

#pragma once

#include "framefilter.h"
#include <cstdint>

// Вызывается из framefilter.cpp когда USE_CUDA определён
void applyFiltersCuda(uint8_t *hostData, int width, int height,
                      const FilterParams &params);


#endif // FRAMEFILTER_CUDA_CUH
