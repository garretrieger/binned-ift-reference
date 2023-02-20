/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/* sfnt table edit utility. */

#include <sstream>
#include <unordered_map>
#include <set>

struct Table {
    Table() {}
    Table(const Table &t) : checksum(t.checksum), offset(t.offset),
                            length(t.length), entryOffset(t.entryOffset),
                            entryNum(t.entryNum) {}
    uint32_t checksum = 0;
    uint32_t offset = 0;
    uint32_t length = 0;

    uint32_t entryOffset = 0;
    uint32_t entryNum = 0;
    // Size of entry in sfnt table
    static const uint32_t entry_size = sizeof(uint32_t) * 4;
    static std::set<uint32_t> known_tables;
};

class sfnt {
 public:
    sfnt() {}
    sfnt(char *buffer, uint32_t length) {
        ss.rdbuf()->pubsetbuf(buffer, length);
    }

    void setBuffer(char *buffer, uint32_t length, bool so = false) {
        ss.rdbuf()->pubsetbuf(buffer, length);
        sfntOnly = so;
    }
    void read();
    void write(bool writeHead = true);
    std::iostream &getTableStream(uint32_t tg, uint32_t &length);
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

    std::unordered_map<uint32_t, Table> directory;
    bool sfntOnly = false, isCff = false, isVariable = false;
    uint32_t headerSum = 0, otherRecordSum = 0, otherTableSum = 0;
    std::stringstream ss;
};
