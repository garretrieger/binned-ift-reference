/*
Copyright 2023 Adobe
All Rights Reserved.

NOTICE: Adobe permits you to use, modify, and distribute this file in
accordance with the terms of the Adobe license agreement accompanying
it.
*/

#include <cassert>

#include "cmap.h"
#include "streamhelp.h"

struct type4seg {
    type4seg() {}
    type4seg(uint16_t ec) : endCode(ec) {}
    uint16_t endCode {0}, startCode {0}, idRangeOffset {0};
    int16_t idDelta {0};
};

bool iftb::readcmap(std::istream &is,
                    std::unordered_map<uint32_t, uint16_t> &uniMap,
                    std::vector<uint16_t> *gidMap) {
    uint16_t numTables, platformID, encodingID;
    uint32_t subtableOffset, candidateOffset = 0;
    readObject<uint16_t>(is);  // version
    readObject(is, numTables);
    for (int i = 0; i < numTables; i++) {
        readObject(is, platformID);
        readObject(is, encodingID);
        readObject(is, subtableOffset);
        if (   (platformID == 0 && encodingID == 4)
            || (platformID == 3 && encodingID == 10)) {
            candidateOffset = subtableOffset;
            break;
        }
        if (   (platformID == 0 && encodingID == 3)
            || (platformID == 3 && encodingID == 1)) {
            if (candidateOffset == 0)
                candidateOffset = subtableOffset;
        }
    }
    if (candidateOffset == 0) {
        std::cerr << "cmap error: No appropriate subtable" << std::endl;
        return false;
    }
    is.seekg(candidateOffset);
    uint16_t format;
    readObject(is, format);
    if (format == 4) {
        uint16_t length, segCount;
        std::vector<type4seg> segs;
        readObject(is, length);
        readObject<uint16_t>(is);  // language
        readObject(is, segCount);  // segCountX2
        segCount /= 2;
        readObject<uint16_t>(is);  // searchRange
        readObject<uint16_t>(is);  // entrySelector
        readObject<uint16_t>(is);  // rangeShift
        segs.reserve(segCount);
        for (uint32_t i = 0; i < segCount; i++)
            segs.emplace_back(readObject<uint16_t>(is));
        readObject<uint16_t>(is);  // reservedPad
        for (uint32_t i = 0; i < segCount; i++)
            readObject(is, segs[i].startCode);
        for (uint32_t i = 0; i < segCount; i++)
            readObject(is, segs[i].idDelta);
        uint32_t firstidroOff = is.tellg();
        for (uint32_t i = 0; i < segCount; i++) {
            readObject(is, segs[i].idRangeOffset);
        }
        int32_t idx = -1;
        for (auto &s: segs) {
            idx++;
            if (s.startCode > s.endCode) {
                std::cerr << "cmap format 4 segment codes out of order";
                std::cerr << std::endl;
                return false;
            }
            for (uint32_t c = s.startCode; c <= s.endCode; c++) {
                uint16_t gid;
                if (s.idRangeOffset != 0) {
                    uint32_t t = s.idRangeOffset + 2 * (c - s.startCode);
                    t += firstidroOff + 2 * idx;
                    is.seekg(t);
                    readObject(is, gid);
                } else
                    gid = c + s.idDelta;
                if (gidMap && gid >= gidMap->size()) {
                    std::cerr << "cmap format 4 bad gid value";
                    std::cerr << std::endl;
                    return false;
                }
                if (gidMap) {
                    uint16_t ck = (*gidMap)[gid];
                    if (ck != 0)
                        uniMap.emplace(c, ck);
                } else
                    uniMap.emplace(c, gid);
            }
        }
    } else if (format == 12) {
        uint32_t length, numGroups;
        readObject<uint16_t>(is);  // reserved
        readObject(is, length);
        readObject<uint32_t>(is);  // language
        readObject(is, numGroups);
        for (uint32_t i = 0; i < numGroups; i++) {
            uint32_t startCharCode, endCharCode, startGlyphID;
            readObject(is, startCharCode);
            readObject(is, endCharCode);
            readObject(is, startGlyphID);
            if (startCharCode > endCharCode) {
                std::cerr << "cmap format 12 char codes out of order";
                std::cerr << std::endl;
                return false;
            }
            for (uint32_t c = startCharCode; c <= endCharCode; c++) {
                uint16_t gid = startGlyphID + c - startCharCode;
                if (gidMap && gid >= gidMap->size()) {
                    std::cerr << "cmap format 12 bad gid value";
                    std::cerr << std::endl;
                    return false;
                }
                if (gidMap) {
                    uint16_t ck = (*gidMap)[gid];
                    if (ck != 0)
                        uniMap.emplace(c, ck);
                } else
                    uniMap.emplace(c, gid);
            }
        }
    } else {
        std::cerr << "cmap error: Chosen subtable has mis-matching format";
        std::cerr << format << std::endl;
        return false;
    }
    if (is.fail()) {
        std::cerr << "cmap error: Stream read failure";
        std::cerr << format << std::endl;
        return false;
    }
    return true;
}
