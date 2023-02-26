/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/* sfnt table edit utility. */

#include <cassert>
#include <stdexcept>
#include <iostream>

#include "sfnt.h"
#include "tag.h"

std::set<uint32_t> Table::known_tables = { T_CMAP, T_CFF,  T_CFF2, T_IFTB,
                                           T_GLYF, T_LOCA, T_GVAR, T_HEAD };

bool sfnt::getTableStream(simplestream &s, uint32_t tg) {
    assert(Table::known_tables.find(tg) != Table::known_tables.end());
    auto i = directory.find(tg);
    if (i == directory.end())
        return false;
    s.rdbuf()->pubsetbuf(ss.bufinfo() + i->second.offset, i->second.length);
    return true;
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

bool sfnt::read() {
    /* Read and validate version */
    ss.seekg(0, std::ios::beg);
    readObject(ss, version);
    switch (version) {
        case 0x00010000: /* 1.0 */
        // case TAG('t', 'r', 'u', 'e'):
        // case TAG('t', 'y', 'p', '1'):
        // case TAG('b', 'i', 't', 's'):
        case tag("OTTO"):
        case T_IFTB:
            break;
        default:
            return error("Unrecognized file type.");
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
    if (ss.fail())
        return error("Stream read failure");
    return true;
}

bool sfnt::write(bool writeHead) {
    uint32_t totalsum = headerSum + otherRecordSum + otherTableSum;
    uint32_t headTableOffset = 0;
    uint32_t checkSumAdjustment;
    if (writeHead and sfntOnly)
        return error("Cannot write to head table in \"sfnt only\" mode");

    for (auto &[tg, table]: directory) {
        if (tg == T_HEAD)
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
            return error("No head table found");
        ss.seekp(headTableOffset + head_adjustment_offset, std::ios::beg);
        writeObject(ss, checkSumAdjustment);
    }
    if (ss.fail())
        return error("Stream write failure.");
    return true;
}

bool sfnt::adjustTable(uint32_t tg, const Table &table, bool rechecksum) {
    assert(Table::known_tables.find(tg) != Table::known_tables.end());
    auto t = directory.find(tg);
    if (t == directory.end())
        return error("Can't find sfnt table to adjust");

    t->second.offset = table.offset;
    t->second.length = table.length;
    if (rechecksum)
        if (!calcTableChecksum(table, t->second.checksum, tg == T_HEAD))
            return false;
    else
        t->second.checksum = table.checksum;
    return true;
}

bool sfnt::calcTableChecksum(const Table &table, uint32_t &checksum,
                             bool is_head) {
    uint32_t headAdjustment, nLongs = (table.length + 3) / 4, p;

    if (sfntOnly)
        return error("Can't calculate table checksum with sfnt header only");

    checksum = 0;
    p = ss.tellg();
    ss.seekg(table.offset);
    while (nLongs--)
        checksum += readObject<uint32_t>(ss);

    if (is_head) {
        /* Adjust sum to ignore head.checkSumAdjustment field */
        ss.seekg(table.offset + head_adjustment_offset, std::ios::beg);
        readObject(ss, headAdjustment);
        checksum -= headAdjustment;
    }
    ss.seekg(p);

    if (ss.fail())
        return error("Stream failure when calculating checksum");
    return true;
}

/* Check that the table checksums and the head adjustment checksums are
   calculated correctly. Also validate the sfnt search fields */
bool sfnt::checkSums(bool full) {
    uint32_t checkSumAdjustment = 0;
    uint32_t totalsum = 0;
    uint32_t headTableOffset = 0;
    uint32_t nHeaderSum = 0;
    uint32_t nOtherTableSum = 0, nOtherRecordSum = 0;
    uint32_t nDirTableSum = 0, nDirRecordSum = 0;
    bool good = true, hasCFF = false;

    /* Read directory header */
    ss.seekg(0);
    uint32_t nLongs = header_size / 4;
    while (nLongs--)
        nHeaderSum += readObject<uint32_t>(ss);

    totalsum += nHeaderSum;

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
        if (tg == T_CFF || tg == T_CFF2)
            hasCFF = true;
        totalsum += tg + 2 * table.checksum + table.offset + table.length;
        bool inDirectory = (directory.find(tg) != directory.end());
        if (!inDirectory) {
            nOtherRecordSum += tg + table.checksum + table.offset +
                               table.length;
            nOtherTableSum += table.checksum;
        } else {
            nDirRecordSum += tg + table.checksum + table.offset +
                             table.length;
            nDirTableSum += table.checksum;
        }
        // ptag(std::cerr, tg);
        // std::cerr << " " << table.entryOffset << " " << table.checksum << " " << table.offset << " " << table.length << std::endl;

        if (tg == T_HEAD)
            headTableOffset = table.offset;

        if (full || inDirectory) {
            if (!calcTableChecksum(table, checksum, tg == T_HEAD))
                return false;
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
    }
    if (headerSum != nHeaderSum)
        std::cerr << "Warning: Mismatch in headerSum" << std::endl;
    if (otherRecordSum != nOtherRecordSum)
        std::cerr << "Warning: Mismatch in otherRecordSum" << std::endl;
    if (otherTableSum != nOtherTableSum)
        std::cerr << "Warning: Mismatch in otherTableSum" << std::endl;
    if (nHeaderSum + nOtherRecordSum + nOtherTableSum + nDirRecordSum +
        nDirTableSum != totalsum) 
        std::cerr << "Warning: totalsum calculation doesn't match" << std::endl;

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
