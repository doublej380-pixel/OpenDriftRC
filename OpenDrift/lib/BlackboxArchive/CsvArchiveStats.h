#pragma once
#include <stddef.h>
#include <stdint.h>

// Streaming parser: state survives SD read boundaries, including a split
// timestamp or header. No per-character filesystem calls or heap allocation.
struct CsvArchiveStats
{
    size_t records = 0;
    uint32_t first = 0, last = 0, timestamp = 0;
    bool header = true, atLineStart = false, timestampValid = false;

    void consume(const uint8_t* bytes, size_t count)
    {
        for(size_t i = 0; i < count; ++i)
        {
            const char value = bytes[i];
            if(header)
            {
                if(value == '\n') { header = false; atLineStart = true; }
                continue;
            }
            if(atLineStart)
            {
                if(value >= '0' && value <= '9')
                {
                    timestamp = timestamp * 10U + (uint32_t)(value - '0');
                    timestampValid = true;
                    continue;
                }
                if(value == ',' && timestampValid)
                {
                    if(records == 0) first = timestamp;
                    last = timestamp;
                }
                atLineStart = false;
            }
            if(value == '\n')
            {
                ++records;
                atLineStart = true;
                timestampValid = false;
                timestamp = 0;
            }
        }
    }
    uint32_t durationMs() const { return records > 1 ? last - first : 0; }
};
