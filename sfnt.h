/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/* sfnt table edit utility. */

#include <cassert>
#include <sstream>
#include <unordered_map>
#include <set>
#include <utility>

#include "streamhelp.h"

struct Table {
    Table() {}
    Table(const Table &t) : checksum(t.checksum), offset(t.offset),
                            length(t.length), entryOffset(t.entryOffset),
                            entryNum(t.entryNum) {}
    uint32_t checksum {0};
    uint32_t offset {0};
    uint32_t length {0};

    uint32_t entryOffset {0};
    uint32_t entryNum {0};

    // Size of entry in sfnt table
    static const uint32_t entry_size = sizeof(uint32_t) * 4;
    static std::set<uint32_t> known_tables;
};

class sfnt {
 public:
    sfnt() {}
    sfnt(char *b, uint32_t l, bool so = false) : sfntOnly(so), ss(b, l) { }
    sfnt(std::string &s, bool so = false) : sfntOnly(so), ss(s.data(), s.size()) { }
    void setBuffer(std::string &s, bool so = false) {
        setBuffer(s.data(), s.size(), so);
    }
    void setBuffer(char *b, uint32_t l, bool so = false) {
        sfntOnly = so;
        ss.rdbuf()->pubsetbuf(b, l);
    }
    void read();
    void write(bool writeHead = true);
    bool has(uint32_t tg) {
        assert(Table::known_tables.find(tg) != Table::known_tables.end());
        return directory.find(tg) != directory.end();
    }
    bool getTableStream(simplestream &s, uint32_t tg);
    uint32_t getTableOffset(uint32_t tg, uint32_t &length);

    void adjustTable(uint32_t tag, const Table &table, bool rechecksum);
    uint32_t calcTableChecksum(const Table &table, bool is_head=false);
    bool checkSums(bool full=false);

 private:
    int32_t version = 0;
    uint16_t numTables = 0;
    static const uint32_t header_size = sizeof(uint32_t) +
                                        sizeof(uint16_t) * 4;
    // head.checkSumAdjustment offset within head table
    static const uint32_t head_adjustment_offset = 2 * sizeof(uint32_t);

    bool sfntOnly = false, isCff = false, isVariable = false;
    uint32_t headerSum = 0, otherRecordSum = 0, otherTableSum = 0;
    char *buffer = NULL;
    uint32_t length = 0;
    simplestream ss;
    std::unordered_map<uint32_t, Table> directory;
};
