#include "table_IFTB.h"

#include <iomanip>
#include <set>

#include "tag.h"

void table_IFTB::writeChunkSet(std::ostream &os, bool seekTo) {
    uint8_t u8 = 0;
    if (seekTo)
        os.seekp(50);
    for (int i = 0; i < chunkCount; i++) {
        if (i && i % 8 == 0) {
            writeObject(os, u8);
            u8 = 0;
        }
        u8 = (u8 << 1) | (chunkSet[i] ? 1 : 0);
    }
    if (chunkCount % 8 != 0) {
        writeObject(os, u8);
    }
}

void table_IFTB::dumpChunkSet(std::ostream &os) {
    os << "chunkSet indexes: ";
    bool printed = false;
    for (uint32_t i = 0; i < chunkCount; i++) {
        if (chunkSet[i]) {
            if (printed)
                os << ", ";
            printed = true;
            os << i;
        }
    }
    os << std::endl;
}

bool table_IFTB::getMissingChunks(const std::vector<uint32_t> &unicodes,
                                  const std::vector<uint32_t> &features,
                                  std::set<uint16_t> &cks) {
    cks.clear();
    uint16_t ck;
    for (auto cp: unicodes) {
        auto i = uniMap.find(cp);
        if (i == uniMap.end())
            continue;
        ck = i->second;
        if (!chunkSet[ck])
            cks.emplace(ck);
    }
    for (auto feat: features) {
        auto i = featureMap.find(feat);
        if (i == featureMap.end())
            continue;
        ck = i->second.startIndex - 1;
        for (auto r: i->second.ranges) {
            ck++;
            assert(r.first <= r.second);
            for (uint16_t j = r.first; j <= r.second; j++) {
                if (chunkSet[j] || cks.find(j) != cks.end()) {
                    cks.emplace(ck);
                    break;
                }
            }
        }
    }
    return true;
}

std::pair<uint32_t, uint32_t> table_IFTB::getChunkRange(uint16_t cidx) {
    if (cidx >= chunkOffsets.size() - 1)
        return std::pair<uint32_t, uint32_t>(0, 0);

    return std::pair<uint32_t, uint32_t>(chunkOffsets[cidx-1],
                                         chunkOffsets[cidx]);
}

            
uint32_t table_IFTB::compile(std::ostream &os, uint32_t offset) {
    uint32_t gidMapTableOffset = 0, chunkOffsetTableOffset = 0;
    uint32_t featureMapTableOffset = 0, relOffsetsOffset = 0;
    os.seekp(offset);
    writeObject(os, majorVersion);
    writeObject(os, minorVersion);
    writeObject(os, (uint32_t) 0);  // reserved
    writeObject(os, id[0]);
    writeObject(os, id[1]);
    writeObject(os, id[2]);
    writeObject(os, id[3]);
    writeObject(os, flags);
    writeObject(os, chunkCount);
    writeObject(os, glyphCount);
    writeObject(os, CFFCharStringsOffset);
    relOffsetsOffset = (uint32_t) os.tellp();
    writeObject(os, gidMapTableOffset);
    writeObject(os, chunkOffsetTableOffset);
    writeObject(os, featureMapTableOffset);
    writeChunkSet(os);
    assert(filesURI.length() < 256);
    writeObject(os, (uint8_t) filesURI.length());
    for (auto c: filesURI)
        writeObject(os, c);
    writeObject(os, (uint8_t) 0);  // Null terminator
    assert(rangeFileURI.length() < 256);
    writeObject(os, (uint8_t) rangeFileURI.length());
    for (auto c: rangeFileURI)
        writeObject(os, c);
    writeObject(os, (uint8_t) 0);  // Null terminator
    gidMapTableOffset = ((uint32_t) os.tellp()) - offset;
    bool writing = false;
    for (uint32_t i = 0; i < glyphCount; i++) {
        if (!writing && gidMap[i] == 0)
            continue;
        else if (!writing) {
            writeObject(os, (uint16_t) i);
            writing = true;
        }
        writeChunkIndex(os, gidMap[i]);
    }
    if (chunkOffsets.size() > 0) {
        assert(chunkOffsets.size() == chunkCount);
        chunkOffsetTableOffset = ((uint32_t) os.tellp()) - offset;
        for (auto i: chunkOffsets)
            writeObject(os, i);
    }
    if (featureMap.size() > 0) {
        featureMapTableOffset = ((uint32_t) os.tellp()) - offset;
        writeObject(os, (uint16_t)featureMap.size());
        for (auto &[t, fm]: featureMap) {
            assert(fm.ranges.size() > 0);
            writeObject(os, t);
            writeChunkIndex(os, fm.startIndex);
            writeChunkIndex(os, fm.ranges.size());
        }
        for (auto &[t, fm]: featureMap) {
            for (auto &[start, end]: fm.ranges) {
                writeChunkIndex(os, start);
                writeChunkIndex(os, end);
            }
        }
    }
    uint32_t l = ((uint32_t) os.tellp()) - offset;
    os.seekp(offset + relOffsetsOffset);
    writeObject(os, gidMapTableOffset);
    writeObject(os, chunkOffsetTableOffset);
    writeObject(os, featureMapTableOffset);
    return l;
}

bool table_IFTB::decompile(std::istream &is, uint32_t offset) {
    uint32_t gidMapTableOffset, chunkOffsetTableOffset;
    uint32_t featureMapTableOffset;
    uint16_t firstMappedGid;
    is.seekg(offset);
    readObject(is, majorVersion);
    if (majorVersion != 0)
        return error("majorVersion != 0, will not read");
    readObject(is, minorVersion);
    if (minorVersion != 1)
        return error("minorVersion != 1, will not read");
    readObject<uint32_t>(is);  // reserved
    readObject(is, id[0]);
    readObject(is, id[1]);
    readObject(is, id[2]);
    readObject(is, id[3]);
    readObject(is, flags);
    readObject(is, chunkCount);
    readObject(is, glyphCount);
    readObject(is, CFFCharStringsOffset);
    readObject(is, gidMapTableOffset);
    readObject(is, chunkOffsetTableOffset);
    readObject(is, featureMapTableOffset);
    uint8_t u8;
    chunkSet.resize(chunkCount);
    uint32_t chunkBytes = chunkCount / 8;
    if (chunkCount % 8 != 0)
        chunkBytes += 1;
    for (uint32_t i=0; i < chunkBytes; i++) {
        readObject(is, u8);
        for (int j = 0; j < 8; j++) {
            if (i * 8 + j >= chunkCount)
                break;
            chunkSet[i * 8 + j] = u8 & 1;
            u8 >>= 1;
        }
    }
    readObject(is, u8);
    filesURI.resize(u8);
    // XXX convert these to read()s
    for (int i = 0; i < u8; i++)
        readObject(is, filesURI[i]);
    readObject<uint8_t>(is);  // Null terminator
    readObject(is, u8);
    rangeFileURI.resize(u8);
    for (int i = 0; i < u8; i++)
        readObject(is, rangeFileURI[i]);
    readObject<uint8_t>(is);  // Null terminator
    is.seekg(offset + gidMapTableOffset);
    readObject(is, firstMappedGid);
    for (uint32_t i = 0; i < glyphCount; i++) {
        if (i < firstMappedGid)
            gidMap.push_back(0);
        else {
            gidMap.push_back(readChunkIndex(is));
        }
    }
    chunkOffsets.clear();
    if (chunkOffsetTableOffset != 0) {
        is.seekg(offset + chunkOffsetTableOffset);
        for (int i = 0; i < chunkCount; i++)
            chunkOffsets.push_back(readObject<uint32_t>(is));
    }
    featureMap.clear();
    if (featureMapTableOffset != 0) {
        is.seekg(offset + featureMapTableOffset);
        uint16_t featureMapCount;
        readObject(is, featureMapCount);
        for (int i = 0; i < featureMapCount; i++) {
            uint32_t feat;
            FeatureMap fm;
            readObject(is, feat);
            fm.startIndex = readChunkIndex(is);
            fm.ranges.resize(readChunkIndex(is));
            featureMap.emplace(feat, std::move(fm));
        }
        for (auto &[_, fm]: featureMap) {
            for (auto &[start, end]: fm.ranges) {
                start = readChunkIndex(is);
                end = readChunkIndex(is);
            }
        }
    }
    if (is.fail())
        return error("decompile stream read failure");
    return true;
}

void table_IFTB::dump(std::ostream &os, bool full) {
    os << "majorVersion: " << majorVersion << std::endl;
    os << "minorVersion: " << minorVersion << std::endl;
    char c = os.fill();
    std::streamsize w = os.width();
    os << "ID: " << std::setfill('0') << std::setw(8) << std::right;
    os << std::hex << id[0] << " " << id[1] << " " << id[2] << " " << id[3];
    os << std::dec << std::setfill(c) << std::setw(w) << std::endl;
    os << "chunkCount: " << chunkCount << std::endl;
    os << "glyphCount: " << glyphCount << std::endl;
    dumpChunkSet(os);
    if (full) {
        os << "gidMap: ";
        bool printed = false;
        for (uint32_t i = 0; i < gidMap.size(); i++) {
            if (!printed)
                os << ", ";
            printed = true;
            os << i << ":" << gidMap[i];
        }
        os << std::endl;
    }
    if (full && chunkOffsets.size() > 0) {
        os << "chunkOffsets: ";
        bool printed = false;
        for (uint32_t i = 0; i < chunkOffsets.size(); i++) {
            if (!printed)
                os << ", ";
            printed = true;
            os << i << ":" << chunkOffsets[i];
        }
        os << std::endl;
    }
    os << "filesURI: " << filesURI << std::endl;
    os << "rangeFileURI: " << rangeFileURI << std::endl;
}

std::string table_IFTB::getChunkURI(uint16_t idx) {
    char buf[10];
    uint8_t digit;
    size_t pos = 0, lastPos = 0;
    snprintf(buf, sizeof(buf), "%08x", (int) idx);

    std::string t;
    while ((pos = filesURI.find('$', pos)) != std::string::npos) {
        t += filesURI.substr(lastPos, pos-lastPos);
        digit = (uint8_t) filesURI[pos+1] - 48;
        if (digit > 8 || digit <= 0) {
            error("Invalid filesURI string in IFTB table");
            t.clear();
            return t;
        }
        t += buf[8 - digit];
        lastPos = pos = pos + 2;
    }
    t += filesURI.substr(lastPos);
    return t;
}
