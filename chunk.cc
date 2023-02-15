
#include <cstring>
#include <vector>

#include "chunk.h"

/* 1, 2, and 4 byte big-endian machine independent output */
template<class T>
static void writeObject(std::ostream &f, const T& o) {
    static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4);
    char b;

    switch (sizeof(T)) {
        case 1:
            b = (uint8_t)(o & 0xFF);
            f.put(b);
            break;
        case 2:
            b = (uint8_t)(o >> 8 & 0xFF);
            f.put(b);
            b = (uint8_t)(o & 0xFF);
            f.put(b);
            break;
        case 4:
            b = (uint8_t)(o >> 24 & 0xFF);
            f.put(b);
            b = (uint8_t)(o >> 16 & 0xFF);
            f.put(b);
            b = (uint8_t)(o >> 8 & 0xFF);
            f.put(b);
            b = (uint8_t)(o & 0xFF);
            f.put(b);
            break;
    }
}

void chunk::write(std::ostream &o, font &f, uint32_t idx, uint32_t table1,
                  uint32_t table2) {
    std::vector<std::vector<char>> blobs1, blobs2;
    std::vector<char> buffer;
    bool twotables = (table2 != 0);
    writeObject(o, (uint16_t) 0);
    writeObject(o, (uint16_t) 1);
    writeObject(o, (uint32_t) 0);
    writeObject(o, idx);
    writeObject(o, (uint32_t) gids.size());
    writeObject(o, (uint8_t) (twotables ? 2 : 1));
    uint32_t gid = HB_SET_VALUE_INVALID;
    while (gids.next(gid)) {
        writeObject(o, (uint16_t) gid);
        unsigned int l, vl;
        const char *vc;
        const char *c = f.get_glyph_content(gid, l, vc, vl);
        buffer.clear();
        if (l > 0) {
            buffer.resize(l);
            memcpy(buffer.data(), c, l);
        }
        blobs1.push_back(std::move(buffer));
        if (twotables) {
            buffer.clear();
            if (vl > 0) {
                buffer.resize(vl);
                memcpy(buffer.data(), vc, vl);
            }
            blobs2.push_back(std::move(buffer));
        }
    }
    writeObject(o, table1);
    uint32_t c_offset = sizeof(uint32_t) * gids.size();
    if (twotables) {
        writeObject(o, table2);
        c_offset *= 2;
    }
    c_offset += sizeof(uint32_t);  // Last offset encodes length
    c_offset += o.tellp();
    for (auto &v: blobs1) {
        writeObject(o, c_offset);
        c_offset += v.size();
    }
    if (twotables) {
        for (auto &v: blobs2) {
            writeObject(o, c_offset);
            c_offset += v.size();
        }
    }
    writeObject(o, c_offset);
    for (auto &v: blobs1)
        o.write(v.data(), v.size());
    if (twotables) {
        for (auto &v: blobs2)
            o.write(v.data(), v.size());
    }
}
