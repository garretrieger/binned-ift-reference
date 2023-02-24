
#include <cassert>
#include <vector>

#include "streamhelp.h"
#include "tag.h"
#include "unchunk.h"

uint16_t chunk_addrecs(std::istream &is, merger &m) {
    uint32_t i32, glyphCount, table1, table2 = 0, offset, lastOffset = 0;
    uint16_t i16, idx;
    uint8_t i8, tableCount;
    std::vector<uint16_t> gids;
    glyphrec gr;

    readObject(is, i16);  // Major version
    if (i16 != 0)
        throw std::runtime_error("Unrecognized chunk major version");
    readObject(is, i16);  // Minor version
    if (i16 != 1)
        throw std::runtime_error("Unrecognized chunk minor version");
    readObject(is, i32);  // Checksum (unused)
    readObject(is, idx);  // Chunk index;
    readObject(is, glyphCount);
    readObject(is, tableCount);
    if (!(tableCount == 1 || tableCount == 2))
        throw std::runtime_error("Chunk table count must be 1 or 2");
    for (int i = 0; i < glyphCount; i++)
        gids.push_back(readObject<uint16_t>(is));
    readObject(is, table1);
    if (tableCount == 2)
        readObject(is, table2);
    m.add_tables(table1, table2);
    readObject(is, lastOffset);
    for (auto i: gids) {
        readObject(is, offset);
        gr.offset = lastOffset;
        gr.length = offset - lastOffset;
        m.glyphMap1.emplace(i, gr);
        lastOffset = offset;
    }
    if (tableCount == 2) {
        for (auto i: gids) {
            readObject(is, offset);
            gr.offset = lastOffset;
            gr.length = offset - lastOffset;
            m.glyphMap2.emplace(i, gr);
            lastOffset = offset;
        }
    }
    return idx;
}

void dump_chunk(std::ostream &os, std::istream &is) {
    uint32_t glyphCount, table1, table2 = 0, offset, lastOffset = 0;
    uint16_t idx;
    uint8_t tableCount;
    std::vector<uint16_t> gids;
    glyphrec gr;

    std::cerr << "Major version: " << readObject<uint16_t>(is) << std::endl;
    std::cerr << "Minor version: " << readObject<uint16_t>(is) << std::endl;
    std::cerr << "Checksum (unused): " << readObject<uint32_t>(is) << std::endl;
    readObject(is, idx);
    std::cerr << "Chunk index: " << idx << std::endl;
    readObject(is, glyphCount);
    std::cerr << "Count of included glyphs: " << glyphCount << std::endl;
    readObject(is, tableCount);
    std::cerr << "Table count (1 or 2): " << tableCount << std::endl;
    std::cerr << "Gid list: ";
    for (int i = 0; i < glyphCount; i++) {
        if (i != 0)
            std::cerr << ", ";
        gids.push_back(readObject<uint16_t>(is));
        std::cerr << gids.back();
    }
    std::cerr << std::endl;
    readObject(is, table1);
    std::cerr << "Table 1: ";
    ptag(std::cerr, table1);
    if (tableCount == 2) {
        std::cerr << ", Table 2: ";
        readObject(is, table2);
        ptag(std::cerr, table2);
    }
    std::cerr << std::endl;
    readObject(is, lastOffset);
    std::cerr << "Table 1 lengths: ";
    for (int i = 0; i < glyphCount; i++) {
        if (i != 0)
            std::cerr << ", ";
        readObject(is, offset);
        std::cerr << (offset - lastOffset);
        lastOffset = offset;
    }
    std::cerr << std::endl;
    if (tableCount == 2) {
        std::cerr << "Table 2 lengths: ";
        for (int i = 0; i < glyphCount; i++) {
            if (i != 0)
                std::cerr << ", ";
            readObject(is, offset);
            std::cerr << (offset - lastOffset);
            lastOffset = offset;
        }
        std::cerr << std::endl;
    }
}
