
#include <cassert>
#include <cstring>
#include <vector>

#include <brotli/decode.h>

#include "streamhelp.h"
#include "tag.h"
#include "unchunk.h"

void merger::chunkAddRecs(uint16_t idx, char *buf, uint32_t len) {
    uint32_t i32, glyphCount, table1, table2 = 0;
    uint32_t offset, lastOffset = 0;
    char *initialOffset;
    uint16_t i16;
    uint8_t i8, tableCount;
    std::vector<uint16_t> gids;
    glyphrec gr;
    simpleistream is;

    is.rdbuf()->pubsetbuf(buf, len);

    readObject(is, i32);
    if (i32 != tag("TIFC"))
        throw std::runtime_error("Initial bytes of chunk must be TIFC");
    readObject(is, i32);
    if (i32 != 0)
        throw std::runtime_error("Reserved bytes in chunk must be 0");
    readObject(is, i32);  // id0
    readObject(is, i32);  // id1
    readObject(is, i32);  // id2
    readObject(is, i32);  // id3
    readObject(is, i32);  // Chunk index;
    if (idx != i32)
        throw std::runtime_error("Chunk index mismatch");
    readObject(is, i32);
    if (len != i32)
        throw std::runtime_error("Chunk length mismatch");
    readObject(is, glyphCount);
    readObject(is, tableCount);
    if (!(tableCount == 1 || tableCount == 2))
        throw std::runtime_error("Chunk table count must be 1 or 2");
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
}

void dumpChunk(std::ostream &os, std::istream &is) {
    uint32_t i32, glyphCount, table1, table2 = 0, idx;
    uint32_t length, offset, lastOffset = 0;
    uint8_t tableCount;
    std::vector<uint16_t> gids;

    readObject(is, i32);
    if (i32 != tag("IFTC")) {
        std::cerr << "Unrecognized chunk type '";
        ptag(std::cerr, i32);
        std::cerr << "': can't display contents" << std::endl;
        return;
    }
    readObject(is, i32);
    if (i32 != 0) {
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

std::filesystem::path getChunkPath(std::filesystem::path &bp,
                                   table_IFTB &tiftb, uint16_t idx) {
    char buf[10];
    uint8_t digit;
    size_t pos = 0, lastPos = 0;
    snprintf(buf, sizeof(buf), "%08x", (int) idx);

    std::filesystem::path t;
    std::string &fURI = tiftb.filesURI;
    while ((pos = fURI.find('$', pos)) != std::string::npos) {
        t += fURI.substr(lastPos, pos-lastPos);
        digit = (uint8_t) fURI[pos+1] - 48;
        if (digit > 8 || digit <= 0)
            throw std::runtime_error("invalid filesURI string in IFTB table");
        t += buf[8 - digit];
        lastPos = pos = pos + 2;
    }
    t += fURI.substr(lastPos);
    return t;
}

std::filesystem::path getRangePath(std::filesystem::path &bp,
                                   table_IFTB &tiftb) {
    std::filesystem::path t(tiftb.rangeFileURI);
    return t;
}

std::string decodeChunk(const std::string &s) {
    uint32_t l;

    std::istringstream ss(s);
    ss.seekg(28);  // length offset
    readObject(ss, l);

    std::string r(l, 0);
    memcpy(r.data(), s.data(), 32);
    size_t decoded_size = l - 32;
    auto ok = BrotliDecoderDecompress(s.size(), (uint8_t *) s.data() + 32,
                                      &decoded_size,
                                      (uint8_t *) r.data() + 32);
    if (ok != BROTLI_DECODER_RESULT_SUCCESS) {
        if (decoded_size != l - 32)
            throw std::runtime_error("Error: Wrong length encoded in chunk");
        else
            throw std::runtime_error("Error: Could not decode IFTB chunk");
    }
    if (decoded_size != l - 32)
        throw std::runtime_error("Error: Wrong length encoded in chunk");

    r[3] = 'C';
    return r;
}

