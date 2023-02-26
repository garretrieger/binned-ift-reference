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

struct tablerec {
    tablerec() {}
    bool used = false;
    bool moving = false;
    uint32_t old_start;
    uint32_t new_start;
    uint32_t old_length;
    uint32_t new_length;
};

struct merger {
    uint32_t table1 = 0, table2 = 0;
    std::map<uint32_t, glyphrec> glyphMap1, glyphMap2;
    void add_tables(uint32_t t1, uint32_t t2) {
        if (table1 == 0) {
            table1 = t1;
            table2 = t2;
        } else {
            assert(table1 == t1 && table2 == t2);
        }
    }
    void chunkAddRecs(uint16_t idx, char *buf, uint32_t len);
};

uint16_t chunkAddRecs(std::istream &is, merger &m);
void dumpChunk(std::ostream &os, std::istream &is);
std::filesystem::path getChunkPath(std::filesystem::path &bp,
                                   table_IFTB &tiftb, uint16_t idx);
std::filesystem::path getRangePath(std::filesystem::path &bp,
                                   table_IFTB &tiftb);
std::string decodeChunk(const std::string &s);
