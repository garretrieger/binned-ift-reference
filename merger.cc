
#include <istream>
#include <vector>

#include "merger.h"
#include "streamhelp.h"

uint16_t merger::addChunkRecs(std::istream &is, uint32_t length) {
    uint32_t i32, glyphCount, table1, table2 = 0, offset, lastOffset = 0;
    uint16_t i16, idx;
    uint8_t i8, tableCount;
    std::vector<uint16_t> gids;
    glyphrec gr;

    readObject(is, i16);  // Major version
    assert(i16 == 0);
    readObject(is, i16);  // Minor version
    assert(i16 == 1);
    readObject(is, i32);  // Checksum (unused)
    readObject(is, idx);  // Chunk index;
    readObject(is, glyphCount);
    readObject(is, tableCount);
    assert(tableCount == 1 || tableCount == 2);
    for (int i = 0; i < glyphCount; i++)
        gids.push_back(readObject<uint16_t>(is));
    readObject(is, table1);
    if (tableCount == 2)
        readObject(is, table2);
    add_tables(table1, table2);
    readObject(is, lastOffset);
    for (auto i: gids) {
        readObject(is, offset);
        gr.offset = lastOffset;
        gr.length = offset - lastOffset;
        glyphMap1.emplace(i, gr);
        lastOffset = offset;
    }
    if (tableCount == 2) {
        for (auto i: gids) {
            readObject(is, offset);
            gr.offset = lastOffset;
            gr.length = offset - lastOffset;
            glyphMap2.emplace(i, gr);
            lastOffset = offset;
        }
    }
    return idx;
}
