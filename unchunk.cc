
#include <cassert>
#include <cstring>
#include <vector>
#include <limits>

#include <brotli/decode.h>
#include <woff2/decode.h>

#include "streamhelp.h"
#include "tag.h"
#include "unchunk.h"

bool merger::chunkAddRecs(uint16_t idx, char *buf, uint32_t len) {
    uint32_t glyphCount, table1, table2 = 0;
    uint32_t offset, lastOffset = 0;
    char *initialOffset;
    uint16_t i16;
    uint8_t i8, tableCount;
    std::vector<uint16_t> gids;
    glyphrec gr;
    simpleistream is;

    is.rdbuf()->pubsetbuf(buf, len);

    if (readObject<uint32_t>(is) != tag("TIFC"))
        return chunkError(idx, "Initial bytes of chunk must be \"TIFC\"");
    if (readObject<uint32_t>(is) != 0)
        return chunkError(idx, "Reserved bytes in chunk must be 0");
    if (readObject<uint32_t>(is) != id0)
        return chunkError(idx, "ID mismatch (id0)");
    if (readObject<uint32_t>(is) != id1)
        return chunkError(idx, "ID mismatch (id1)");
    if (readObject<uint32_t>(is) != id2)
        return chunkError(idx, "ID mismatch (id2)");
    if (readObject<uint32_t>(is) != id3)
        return chunkError(idx, "ID mismatch (id3)");
    if (readObject<uint32_t>(is) != idx)
        return chunkError(idx, "Chunk index mismatch");
    if (readObject<uint32_t>(is) != len)
        return chunkError(idx, "Chunk index mismatch");
    readObject(is, glyphCount);
    if (glyphCount > std::numeric_limits<uint16_t>::max())
        return chunkError(idx, "Unreasonable glyph count");
    readObject(is, tableCount);
    if (!(tableCount == 1 || tableCount == 2))
        return chunkError(idx, "Chunk table count must be 1 or 2");
    for (int i = 0; i < glyphCount; i++)
        gids.push_back(readObject<uint16_t>(is));
    readObject(is, table1);
    if (tableCount == 2)
        readObject(is, table2);
    add_tables(table1, table2);
    readObject(is, lastOffset);
    for (auto i: gids) {
        readObject(is, offset);
        gr.offset = buf + lastOffset;
        gr.length = offset - lastOffset;
        glyphMap1.emplace(i, gr);
        lastOffset = offset;
    }
    if (tableCount == 2) {
        for (auto i: gids) {
            readObject(is, offset);
            gr.offset = buf + lastOffset;
            gr.length = offset - lastOffset;
            glyphMap2.emplace(i, gr);
            lastOffset = offset;
        }
    }
    if (is.fail())
        return chunkError(idx, "Decompile stream read failure");
    return true;
}

void dumpChunk(std::ostream &os, std::istream &is) {
    uint32_t u32, glyphCount, table1, table2 = 0, idx;
    uint32_t length, offset, lastOffset = 0;
    uint8_t tableCount;
    std::vector<uint16_t> gids;

    readObject(is, u32);
    if (u32 != tag("IFTC")) {
        std::cerr << "Unrecognized chunk type '";
        ptag(std::cerr, u32);
        std::cerr << "': can't display contents" << std::endl;
        return;
    }
    readObject(is, u32);
    if (u32 != 0) {
        std::cerr << "Reserved bytes in chunk must be 0: ";
        std::cerr << "can't display contents" << std::endl;
    }
    char c = os.fill();
    std::streamsize w = os.width();
    os << "ID: " << std::setfill('0') << std::setw(8) << std::right;
    os << std::hex << readObject<uint32_t>(is) << " ";
    os << readObject<uint32_t>(is) << " ";
    os << readObject<uint32_t>(is) << " ";
    os << readObject<uint32_t>(is) << std::dec << std::setfill(c);
    os << std::setw(w) << std::endl;
    readObject(is, idx);
    std::cerr << "Chunk index: " << idx << std::endl;
    readObject(is, length);
    std::cerr << "Uncompressed length: " << length << std::endl;
    readObject(is, glyphCount);
    std::cerr << "Count of included glyphs: " << glyphCount << std::endl;
    readObject(is, tableCount);
    std::cerr << "Table count (1 or 2): " << (int) tableCount << std::endl;
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
 
std::string decodeChunk(char *buf, size_t length) {
    uint32_t l;

    simpleistream sis;
    sis.rdbuf()->pubsetbuf(buf, length);
    sis.seekg(28);  // length offset
    readObject(sis, l);

    std::string r(l, 0);
    memcpy(r.data(), buf, 32);
    size_t decoded_size = l - 32;
    auto ok = BrotliDecoderDecompress(length - 32, (uint8_t *) buf + 32,
                                      &decoded_size,
                                      (uint8_t *) r.data() + 32);
    if (ok != BROTLI_DECODER_RESULT_SUCCESS && decoded_size == l - 32) {
        std::cerr << "Error: Could not decompress IFTZ chunk";
        r.clear();
    } else if (decoded_size != l - 32) {
        std::cerr << "Error: Wrong length encoded in chunk";
        r.clear();
    }

    r[3] = 'C';
    return r;
}


/* When buf != NULL the output should be copied into the string, which
   will be replaced.
   When buf == NULL the string will be treated as the input, which will
   only be changed if it needs to be decoded (decompressed)
 */
uint32_t decodeBuffer(char *buf, uint32_t length, std::string &s,
                     float reserveExtra) {
    bool is_woff2 = false, is_compressed_chunk = false;
    bool in_string = (buf == NULL);
    uint32_t tg; 

    assert(reserveExtra >= 0 && reserveExtra < 10.0);

    if (in_string) {
        buf = s.data();
        length = s.size();
    }
    tg = tagFromBuffer(buf);

    if (tg == tag("wOF2"))
        is_woff2 = true;
    else if (tg == tag("IFTZ"))
        is_compressed_chunk = true;

    if (is_woff2) {
        const uint8_t *iptr = reinterpret_cast<const uint8_t*>(buf);
        uint32_t sz = std::min(woff2::ComputeWOFF2FinalSize(iptr, length),
                               woff2::kDefaultMaxSize);
        uint32_t reserve = (uint32_t) ((float) sz * (1 + reserveExtra));
        std::string t;
        t.reserve(reserve);
        t.resize(sz);
        woff2::WOFF2StringOut o(&t);
        auto ok = woff2::ConvertWOFF2ToTTF(iptr, length, &o);
        if (ok) {
            s.swap(t);
            t.clear();
            tg = tagFromBuffer(s.data());
        } else {
            tg = 0;
        }
    } else if (is_compressed_chunk) {
        std::string t = decodeChunk(buf, length);
        if (t.size() > 4) {
            s.swap(t);
            t.clear();
            tg = tagFromBuffer(s.data());
        } else {
            tg = 0;
        }
    }
    return tg;
}
