/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/* sfnt table edit utility. */

#include <cassert>
#include <stdexcept>
#include <iostream>

#include "sfnt.h"
#include "streamhelp.h"
#include "tag.h"

std::set<uint32_t> Table::known_tables = { tag("cmap"), tag("CFF "),
                                           tag("CFF2"), tag("IFTC"),
                                           tag("glyf"), tag("loca"),
                                           tag("gvar"), tag("head") };

std::iostream &sfnt::getTableStream(uint32_t tg, uint32_t &length) {
    assert(Table::known_tables.find(tg) != Table::known_tables.end());
    auto i = directory.find(tg);
    if (i != directory.end()) {
        ss.seekg(i->second.offset, std::ios::beg);
        ss.seekp(i->second.offset, std::ios::beg);
        length = i->second.length;
    } else {
        length = 0;
    }
    return ss;
}

uint32_t sfnt::getTableOffset(uint32_t tg, uint32_t &length) {
    assert(Table::known_tables.find(tg) != Table::known_tables.end());
    uint32_t r = 0;
    auto i = directory.find(tg);
    if (i != directory.end()) {
        r = i->second.offset;
        length = i->second.length;
    } else {
        length = 0;
    }
    return r;
}

void sfnt::read() {
    /* Read and validate version */
    ss.seekg(0, std::ios::beg);
    readObject(ss, version);
    switch (version) {
        case 0x00010000: /* 1.0 */
        // case TAG('t', 'r', 'u', 'e'):
        // case TAG('t', 'y', 'p', '1'):
        // case TAG('b', 'i', 't', 's'):
        case tag("OTTO"):
            break;
        default:
            throw std::runtime_error("Unrecognized sfnt type.");
    }

    readObject(ss, numTables);

    // header checksum
    ss.seekg(0, std::ios::beg);
    uint32_t nLongs = header_size / 4;
    while (nLongs--)
        headerSum += readObject<uint32_t>(ss);

    otherTableSum = otherRecordSum = 0;

    for (int i = 0; i < numTables; i++) {
        uint32_t tg;
        Table table;
        table.entryNum = i;
        table.entryOffset = ss.tellg();
        readObject(ss, tg);
        readObject(ss, table.checksum);
        readObject(ss, table.offset);
        readObject(ss, table.length);
        if (Table::known_tables.find(tg) != Table::known_tables.end())
            directory.emplace(tg, table);
        else {
            otherRecordSum += tg + table.checksum + table.offset +
                              table.length;
            otherTableSum += table.checksum;
        }
    }
}

void sfnt::write(bool writeHead) {
    uint32_t totalsum = headerSum + otherRecordSum + otherTableSum;
    uint32_t headTableOffset = 0;
    uint32_t checkSumAdjustment;
    if (writeHead and sfntOnly)
        throw std::runtime_error("Can't update head table "
                                 "with sfnt header only.");

    for (auto &[tg, table]: directory) {
        if (tg == tag("head"))
            headTableOffset = table.offset;
        ss.seekp(table.entryOffset, std::ios::beg);
        writeObject(ss, tg);
        writeObject(ss, table.checksum);
        writeObject(ss, table.offset);
        writeObject(ss, table.length);
        totalsum += tg + 2 * table.checksum + table.offset + table.length;
    }

    checkSumAdjustment = 0xb1b0afba - totalsum;

    if (writeHead) {
        if (headTableOffset == 0)
            throw std::runtime_error(" Warning: No head table found");
        ss.seekp(headTableOffset + head_adjustment_offset, std::ios::beg);
        writeObject(ss, checkSumAdjustment);
    }
}

void sfnt::adjustTable(uint32_t tg, const Table &table, bool rechecksum) {
    assert(Table::known_tables.find(tg) != Table::known_tables.end());
    auto t = directory.find(tg);
    if (t == directory.end())
        throw std::runtime_error("Can't find sfnt table to adjust");

    t->second.offset = table.offset;
    t->second.length = table.length;
    if (rechecksum)
        t->second.checksum = calcTableChecksum(table, tg == tag("head"));
    else
        t->second.checksum = table.checksum;
}

uint32_t sfnt::calcTableChecksum(const Table &table, bool is_head) {
    uint32_t checksum = 0, headAdjustment;
    uint32_t nLongs = (table.length + 3) / 4;

    if (sfntOnly)
        throw std::runtime_error("Can't calculate table checksum "
                                 "with sfnt header only.");

    ss.seekg(table.offset, std::ios::beg);
    while (nLongs--)
        checksum += readObject<uint32_t>(ss);

    if (is_head) {
        /* Adjust sum to ignore head.checkSumAdjustment field */
        ss.seekg(table.offset + head_adjustment_offset, std::ios::beg);
        readObject(ss, headAdjustment);
        checksum -= headAdjustment;
    }
    return checksum;
}

/* Check that the table checksums and the head adjustment checksums are
   calculated correctly. Also validate the sfnt search fields */
bool sfnt::checkSums(bool full) {
    uint32_t checkSumAdjustment = 0;
    uint32_t totalsum = 0;
    uint32_t headTableOffset = 0;
    bool good = true;

    /* Read directory header */
    ss.seekg(0, std::ios::beg);
    uint32_t nLongs = (header_size + Table::entry_size * numTables) / 4;
    while (nLongs--)
        totalsum += readObject<uint32_t>(ss);

    ss.seekg(header_size, std::ios::beg);
    for (int i = 0; i < numTables; i++) {
        uint32_t checksum;
        uint32_t tg;
        Table table;
        table.entryNum = i;
        table.entryOffset = ss.tellg();
        readObject(ss, tg);
        readObject(ss, table.checksum);
        readObject(ss, table.offset);
        readObject(ss, table.length);

        if (tg == tag("head"))
            headTableOffset = table.offset;

        if (full || directory.find(tg) != directory.end()) {
            checksum = calcTableChecksum(table, tg == tag("head"));
            if (table.checksum != checksum) {
                good = false;
                std::cerr << "Warning: '";
                ptag(std::cerr, tg);
                std::cerr << "' bad checksum (found=";
                std::cerr << std::hex << table.checksum;
                std::cerr << ", calculated=";
                std::cerr << std::hex << checksum;
                std::cerr << std::endl;
            }
        }

        totalsum += table.checksum;
    }
    
    if (headTableOffset == 0) {
        good = false;
        std::cerr << "Warning: No head table found" << std::endl;
    } else {
        ss.seekg(headTableOffset + head_adjustment_offset, std::ios::beg);
        readObject(ss, checkSumAdjustment);
    }

    if (good && checkSumAdjustment != 0xb1b0afba - totalsum) {
        std::cerr << "Warning: bad head checkSumAdjustment (found=";
        std::cerr << std::hex << checkSumAdjustment;
        std::cerr << ", calculated=";
        std::cerr << std::hex << totalsum;
        std::cerr << std::endl;
        good = false;
    }
    return good;
}