#include "tile_render_instrumentation.h"

#include <debugger.h>

TileRenderMetrics &TileRenderMetrics::operator+=(const TileRenderMetrics &other)
{
    scannedCells += other.scannedCells;
    nonEmptyTiles += other.nonEmptyTiles;
    renderedTiles += other.renderedTiles;
    tileLayerNanoseconds += other.tileLayerNanoseconds;
    rangeNanoseconds += other.rangeNanoseconds;
    tilesetLookupNanoseconds += other.tilesetLookupNanoseconds;
    atlasLookupNanoseconds += other.atlasLookupNanoseconds;
    blitNanoseconds += other.blitNanoseconds;
    dirtyRegionNanoseconds += other.dirtyRegionNanoseconds;
    mapDrawNanoseconds += other.mapDrawNanoseconds;
    return *this;
}

void TileRenderInstrumentation::Initialize(uint32_t reportIntervalFrames)
{
    std::lock_guard<std::mutex> lock(mutex_);
    reportIntervalFrames_ = reportIntervalFrames;
    framesSinceReport_ = 0;
    metrics_ = {};
}

bool TileRenderInstrumentation::IsEnabled()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return reportIntervalFrames_ != 0;
}

void TileRenderInstrumentation::RecordFrame(const TileRenderMetrics &metrics)
{
    uint32_t reportedFrames = 0;
    TileRenderMetrics reportedMetrics;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (reportIntervalFrames_ == 0)
            return;

        ++framesSinceReport_;
        metrics_ += metrics;
        if (framesSinceReport_ < reportIntervalFrames_)
            return;

        reportedFrames = framesSinceReport_;
        reportedMetrics = metrics_;
        framesSinceReport_ = 0;
        metrics_ = {};
    }

    const double frames = reportedFrames;
    const auto averageMilliseconds = [frames](uint64_t nanoseconds)
    {
        return static_cast<double>(nanoseconds) / frames / 1e6;
    };
    const double scannedCells =
        static_cast<double>(reportedMetrics.scannedCells) / frames;
    const double nonEmptyTiles =
        static_cast<double>(reportedMetrics.nonEmptyTiles) / frames;
    const double renderedTiles =
        static_cast<double>(reportedMetrics.renderedTiles) / frames;

    Debugger::Instance().LogInfo(
        "Tile render summary (" + std::to_string(reportedFrames) +
        " frames): map draw " +
        std::to_string(averageMilliseconds(reportedMetrics.mapDrawNanoseconds)) +
        " ms/frame, tile layers " +
        std::to_string(averageMilliseconds(reportedMetrics.tileLayerNanoseconds)) +
        " ms/frame; range " +
        std::to_string(averageMilliseconds(reportedMetrics.rangeNanoseconds)) +
        " ms, tileset lookup " +
        std::to_string(averageMilliseconds(reportedMetrics.tilesetLookupNanoseconds)) +
        " ms, atlas lookup " +
        std::to_string(averageMilliseconds(reportedMetrics.atlasLookupNanoseconds)) +
        " ms, blit " +
        std::to_string(averageMilliseconds(reportedMetrics.blitNanoseconds)) +
        " ms, dirty region " +
        std::to_string(averageMilliseconds(reportedMetrics.dirtyRegionNanoseconds)) +
        " ms per frame; " + std::to_string(scannedCells) +
        " cells scanned, " + std::to_string(nonEmptyTiles) +
        " non-empty, " + std::to_string(renderedTiles) + " rendered per frame");
}
