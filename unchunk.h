#include <iostream>
#include <filesystem>
#include <cassert>
#include <map>

#include "table_IFTB.h"

#pragma once

struct glyphrec {
    glyphrec() {}
    glyphrec(const char *o, uint32_t l): offset(o), length(l) {}
    const char *offset = NULL;
    uint32_t length = 0;
};

struct merger {
    uint32_t table1 {0}, table2 {0}, id0 {0}, id1 {0}, id2 {0}, id3 {0};
    std::map<uint32_t, glyphrec> glyphMap1, glyphMap2;
    void add_tables(uint32_t t1, uint32_t t2) {
        if (table1 == 0) {
            table1 = t1;
            table2 = t2;
        } else {
            assert(table1 == t1 && table2 == t2);
        }
    }
    bool chunkAddRecs(uint16_t idx, char *buf, uint32_t len);
    void reset() {
        table1 = table2 = 0;
        glyphMap1.clear();
        glyphMap2.clear();
    }
    void setID(uint32_t i0, uint32_t i1, uint32_t i2, uint32_t i3) {
        id0 = i0;
        id1 = i1;
        id2 = i2;
        id3 = i3;
    }
    bool chunkError(uint16_t cidx, const char *m) {
        std::cerr << "Chunk " << cidx << " error: " << m << std::endl;
        return false;
    }
};

void dumpChunk(std::ostream &os, std::istream &is);
std::string decodeChunk(char *buf, size_t length);
uint32_t decodeBuffer(char *buf, uint32_t length, std::string &s,
                      float reserveExtra = 0.0);
