
#include <cassert>
#include <map>

#pragma once

struct glyphrec {
    glyphrec() {}
    glyphrec(uint32_t o, uint32_t l): offset(o), length(l) {}
    uint32_t offset = 0;
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
};
