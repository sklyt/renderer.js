#pragma once

#include <cstdint>
#include <mutex>

struct TileRenderMetrics
{
    uint64_t scannedCells = 0;
    uint64_t nonEmptyTiles = 0;
    uint64_t renderedTiles = 0;
    uint64_t tileLayerNanoseconds = 0;
    uint64_t rangeNanoseconds = 0;
    uint64_t tilesetLookupNanoseconds = 0;
    uint64_t atlasLookupNanoseconds = 0;
    uint64_t blitNanoseconds = 0;
    uint64_t dirtyRegionNanoseconds = 0;
    uint64_t mapDrawNanoseconds = 0;

    TileRenderMetrics &operator+=(const TileRenderMetrics &other);
};

class TileRenderInstrumentation
{
public:
    void Initialize(uint32_t reportIntervalFrames);
    bool IsEnabled();
    void RecordFrame(const TileRenderMetrics &metrics);

private:
    std::mutex mutex_;
    uint32_t reportIntervalFrames_ = 0;
    uint32_t framesSinceReport_ = 0;
    TileRenderMetrics metrics_;
};
