/*
Copyright 2023 Adobe
All Rights Reserved.

NOTICE: Adobe permits you to use, modify, and distribute this file in
accordance with the terms of the Adobe license agreement accompanying
it.
*/

#include <sstream>
#include <cstring>

#include <brotli/encode.h>

#include "chunk.h"
#include "streamhelp.h"
#include "tag.h"

void iftb::chunk::compile(std::ostream &os, uint16_t idx,
                          uint32_t *id,
                          hb_set_t* gids,
                          uint32_t table1,
                          std::vector<iftb::merger::glyphrec> &recs1,
                          uint32_t table2,
                          std::vector<iftb::merger::glyphrec> &recs2,
                          uint32_t offset) {
    unsigned int length1, length2;
    bool twotables = (table2 != 0);
    os.seekp(offset);
    writeObject(os, tag("IFTC"));
    writeObject(os, (uint32_t) 0);  // reserved
    writeObject(os, id[0]);
    writeObject(os, id[1]);
    writeObject(os, id[2]);
    writeObject(os, id[3]);
    writeObject(os, (uint32_t) idx);
    uint32_t bloffset = (uint32_t) os.tellp();
    writeObject(os, (uint32_t) 0);  // length
    writeObject(os, (uint32_t) hb_set_get_population(gids));
    writeObject(os, (uint8_t) (twotables ? 2 : 1));
    uint32_t gid = HB_SET_VALUE_INVALID;
    while (hb_set_next(gids, &gid))
        writeObject(os, (uint16_t) gid);
    writeObject(os, table1);
    uint32_t c_offset = sizeof(uint32_t) * hb_set_get_population(gids);
    if (twotables) {
        writeObject(os, table2);
        c_offset *= 2;
    }
    c_offset += sizeof(uint32_t);  // Last offset encodes length
    c_offset += os.tellp();
    gid = HB_SET_VALUE_INVALID;
    while (hb_set_next(gids, &gid)) {
        writeObject(os, c_offset);
        c_offset += recs1[gid].length;
    }
    if (twotables) {
        gid = HB_SET_VALUE_INVALID;
        while (hb_set_next(gids, &gid)) {
            writeObject(os, c_offset);
            c_offset += recs2[gid].length;
        }
    }
    writeObject(os, c_offset);
    gid = HB_SET_VALUE_INVALID;
    while (hb_set_next(gids, &gid))
        os.write(recs1[gid].offset, recs1[gid].length);
    if (twotables) {
        gid = HB_SET_VALUE_INVALID;
        while (hb_set_next(gids, &gid))
            os.write(recs2[gid].offset, recs2[gid].length);
    }
    uint32_t bl = (uint32_t) os.tellp() - offset;
    os.seekp(bloffset);
    writeObject(os, bl);
}

std::string iftb::chunk::encode(std::stringstream &ss) {
    uint32_t l;

    ss.seekg(28);  // length offset
    
    readObject(ss, l);
    std::string s(ss.str());

    if (s.size() != l)
        throw std::runtime_error("Length discrepancy in uncompressed chunk");

    size_t encoded_size = BrotliEncoderMaxCompressedSize(l - 32);
    std::string z(32 + encoded_size, 0);
    memcpy(z.data(), s.data(), 32);
    if (!BrotliEncoderCompress(BROTLI_MAX_QUALITY, BROTLI_DEFAULT_WINDOW,
                               BROTLI_MODE_FONT, l-32,
                               (const uint8_t *) s.data() + 32,
                               &encoded_size, (uint8_t *) z.data() + 32))
         throw std::runtime_error("Could not compress chunk");
    z.resize(32 + encoded_size);
    z[3] = 'Z';
    return z;
}
