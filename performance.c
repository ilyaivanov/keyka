#pragma once
#include <windows.h>
#include "types.h"

typedef enum PerfMetric
{
    Overall,
    Memory,
    Draw,
    DiBits,
    TotalMetrics
} PerfMetric;

u64 frequency;
u64 starts[TotalMetrics];
u64 ends[TotalMetrics];

u32 GetMicrosecondsFor(PerfMetric metric)
{
    return (u32)((f32)((ends[metric] - starts[metric]) * 1000 * 1000) / (f32)frequency);
}

void StartMetric(PerfMetric metric)
{
    LARGE_INTEGER start = {0};
    QueryPerformanceCounter(&start);
    starts[metric] = start.QuadPart;
}

u32 EndMetric(PerfMetric metric)
{
    LARGE_INTEGER end = {0};
    QueryPerformanceCounter(&end);
    ends[metric] = end.QuadPart;
    return GetMicrosecondsFor(metric);
}

void InitPerf()
{
    LARGE_INTEGER fre = {0};
    QueryPerformanceFrequency(&fre);   
    frequency = fre.QuadPart;
    StartMetric(Overall);
}

inline void EndFrame()
{
    starts[Overall] = ends[Overall];
}
