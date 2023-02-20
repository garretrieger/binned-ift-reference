
#include <cstring>
#include <sstream>
#include <vector>

#include "chunk.h"
#include "streamhelp.h"

uint16_t chunk::addrecs(std::istream &is, uint32_t length, merger &m) {
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

void chunk::compile(std::ostream &os, font &f, uint16_t idx,
                    uint32_t table1, uint32_t table2, uint32_t offset) {
    std::vector<glyphrec> blobs1, blobs2;
    unsigned int l, vl;
    const char *base, *c, *vc;
    bool twotables = (table2 != 0);

    base = f.get_glyph_content(0, l, vc, vl);
    os.seekp(offset);
    writeObject(os, (uint16_t) 0);
    writeObject(os, (uint16_t) 1);
    writeObject(os, (uint32_t) 0);
    writeObject(os, idx);
    writeObject(os, (uint32_t) gids.size());
    writeObject(os, (uint8_t) (twotables ? 2 : 1));
    uint32_t gid = HB_SET_VALUE_INVALID;
    while (gids.next(gid)) {
        writeObject(os, (uint16_t) gid);
        const char *c = f.get_glyph_content(gid, l, vc, vl);
        blobs1.emplace_back(c - base, l);
        if (twotables)
            blobs2.emplace_back(vc - base, vl);
    }
    writeObject(os, table1);
    uint32_t c_offset = sizeof(uint32_t) * gids.size();
    if (twotables) {
        writeObject(os, table2);
        c_offset *= 2;
    }
    c_offset += sizeof(uint32_t);  // Last offset encodes length
    c_offset += os.tellp();
    for (auto &b: blobs1) {
        writeObject(os, c_offset);
        c_offset += b.length;
    }
    if (twotables) {
        for (auto &b: blobs2) {
            writeObject(os, c_offset);
            c_offset += b.length;
        }
    }
    writeObject(os, c_offset);
    for (auto &b: blobs1)
        os.write(base + b.offset, b.length);
    if (twotables) {
        for (auto &b: blobs2)
            os.write(base + b.offset, b.length);
    }
}
