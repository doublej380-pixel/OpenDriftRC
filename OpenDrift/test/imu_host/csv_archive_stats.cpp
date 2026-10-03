#include "CsvArchiveStats.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <string>
int main()
{
    const std::string text = std::string(5000, 'h') + "\n1000," + std::string(5000, 'x') + "\n1050,a\n1100,b\n";
    for(size_t chunk : {1U, 7U, 4096U})
    {
        CsvArchiveStats stats;
        for(size_t offset = 0; offset < text.size(); offset += chunk)
            stats.consume(reinterpret_cast<const uint8_t*>(text.data() + offset),
                std::min(chunk, text.size() - offset));
        assert(stats.records == 3 && stats.durationMs() == 100);
    }
    CsvArchiveStats wrap;
    const std::string text2 = "time_ms,yaw\n4294967290,0\n10,0\n";
    wrap.consume(reinterpret_cast<const uint8_t*>(text2.data()), text2.size());
    assert(wrap.records == 2 && wrap.durationMs() == 16);
    std::puts("CSV archive statistics tests passed");
}
