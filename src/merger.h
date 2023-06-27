/*
Copyright 2023 Adobe
All Rights Reserved.

NOTICE: Adobe permits you to use, modify, and distribute this file in
accordance with the terms of the Adobe license agreement accompanying
it.
*/

/* iftb::merger contains the code to merge chunk data into an IFTB encoded
   font and update the IFTB table accordingly. It is also included in the
   encoder for, e.g., preloading.
 */

#include <iostream>
#include <filesystem>
#include <cassert>
#include <map>

#include "table_IFTB.h"
#include "sfnt.h"

#pragma once

namespace iftb {
    class merger;
    void dumpChunk(std::ostream &os, std::istream &is);
    std::string decodeChunk(char *buf, size_t length);
    uint32_t decodeBuffer(char *buf, uint32_t length, std::string &s,
                          float reserveExtra = 0.0);
}

class iftb::merger {
public:
    struct glyphrec {
        glyphrec() {}
        glyphrec(const char *o, uint32_t l): offset(o), length(l) {}
        const char *offset = NULL;
        uint32_t length = 0;
    };
    void add_tables(uint32_t t1, uint32_t t2) {
        if (table1 == 0) {
            table1 = t1;
            table2 = t2;
        } else {
            assert(table1 == t1 && table2 == t2);
        }
    }
    std::string &stringForChunk(uint16_t idx) {
        auto i = chunkData.emplace(idx, "");
        return i.first->second;
    }
    bool chunkAddRecs(uint16_t idx, const std::string &s);
    bool unpackChunks();
    void reset() {
        table1 = table2 = 0;
        glyphMap1.clear();
        glyphMap2.clear();
        chunkData.clear();
        has_cff = is_cff2 = false;
        glyphCount = charStringOff = gvarDataOff = 0;
        t1tag = t1off = t1clen = t1nlen = 0;
        glyfcoff = glyfnoff = glyfclen = glyfnlen = 0;
        locacoff = locanoff = localen = fontend = 0;
    }
    void setID(uint32_t i[4]) {
        id[0] = i[0];
        id[1] = i[1];
        id[2] = i[2];
        id[3] = i[3];
    }
    bool hasChunk(uint16_t idx) {
        return chunkData.find(idx) != chunkData.end();
    }
    uint32_t calcLengthDiff(std::istream &is, uint32_t glyphCount,
                            std::map<uint16_t, glyphrec> &glyphMap);
    bool copyGlyphData(std::iostream &is, uint32_t glyphCount,
                       char *nbase, char *cbase, uint32_t ldiff,
                       std::map<uint16_t, glyphrec> &glyphMap,
                       uint32_t basediff);
    uint32_t calcLayout(iftb::sfnt &sf, uint32_t numg, uint32_t cso);
    bool merge(iftb::sfnt &sf, char *oldbuf, char *newbuf);
private:
    bool chunkError(uint16_t cidx, const char *m) {
        std::cerr << "Chunk " << cidx << " error: " << m << std::endl;
        return false;
    }
    uint32_t table1 {0}, table2 {0}, id[4] {0,0,0,0};
    std::map<uint16_t, glyphrec> glyphMap1, glyphMap2;
    std::map<uint16_t, std::string> chunkData;
    simplestream ss;

    // These bridge between calcLayout() and merge()
    bool has_cff {false}, is_cff2 {false};
    uint32_t glyphCount {0}, charStringOff {0}, gvarDataOff {0};
    uint32_t t1tag {0}, t1off {0}, t1clen {0}, t1nlen {0};
    uint32_t glyfcoff {0}, glyfnoff {0}, glyfclen {0}, glyfnlen {0};
    uint32_t locacoff {0}, locanoff {0}, localen {0}, fontend {0};
};

