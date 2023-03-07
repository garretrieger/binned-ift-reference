#include "client.h"
#include "streamhelp.h"
#include "tag.h"

bool iftb::client::loadFont(std::string &s, bool keepGIDMap) {
    return loadFont(s.data(), s.length(), keepGIDMap);
}

bool iftb::client::loadFont(char *buf, uint32_t length, bool keepGIDMap) {
    uint32_t tg = iftb::decodeBuffer(buf, length, fontData, extraPercent());
    if (tg != 0x00010000 && tg != tag("OTTO") && tg != tag("IFTB"))
        return error("Unrecognized font type.");

    sfnt.setBuffer(fontData);
    if (!sfnt.read()) {
        failed = true;
        return false;
    }

    if (!sfnt.getTableStream(ss, T_IFTB))
        return error("No IFTB table in font");

    if (!tiftb.decompile(ss)) {
        failed = true;
        return false;
    }

    if (!sfnt.getTableStream(ss, T_CMAP))
        return error("No cmap table in font");

    if (!tiftb.addcmap(ss, keepGIDMap)) {
        failed = true;
        return false;
    }
    merger.setID(tiftb.getID());

    return true;
}

bool iftb::client::addChunk(uint16_t idx, char *buf, uint32_t length,
                            bool setPending) {
    if (!hasFont() or failed)
        return false;
    if (idx >= tiftb.getChunkCount())
        return error("Chunk index exceeds chunk count");
    if (setPending) {
        pendingChunks.insert(idx);
    } else if (pendingChunks.find(idx) == pendingChunks.end()) {
        return error("Cannot add chunk index that is not pending");
    }
    uint32_t tg = iftb::decodeBuffer(buf, length, merger.stringForChunk(idx));
    if (tg != tag("IFTC"))
        return error("File type for chunk is not IFTC");
    return true;
}

bool iftb::client::canMerge() {
    for (auto i: pendingChunks)
        if (!merger.hasChunk(i)) {
            std::cerr << "Can't merge: missing chunk " << i << std::endl;
            return false;
        }
    return true;
}

bool iftb::client::merge(bool asIFTB) {
    std::string newString;
    bool swapping = false;

    char *newBuf = fontData.data();
    if (!canMerge())
        return false;

    if (!merger.unpackChunks())
        return false;

    uint32_t newLength = merger.calcLayout(sfnt, tiftb.getGlyphCount(),
                                      tiftb.getCharStringOffset());
    if (newLength == 0)
        return false;
    if (newLength > fontData.capacity()) {
        swapping = true;
        newString.reserve((uint32_t) ((float)newLength *
                                      (1 + extraPercent())));
        newString.resize(newLength, 0);
        newBuf = newString.data();
    } else {
        fontData.resize(newLength, 0);
    }
    // merge method reassigns sfnt's buffer.
    if (!merger.merge(sfnt, fontData.data(), newBuf))
        return false;
    tiftb.updateChunkSet(pendingChunks);
    if (!sfnt.getTableStream(ss, T_IFTB))
        return false;
    tiftb.writeChunkSet(ss, true);
    if (!sfnt.recalcTableChecksum(T_IFTB))
        return false;
    if (!sfnt.write(asIFTB))
        return false;
    isIFTB = asIFTB;
    if (swapping)
        fontData.swap(newString);
    merger.reset();
    pendingChunks.clear();
    return true;
}

uint16_t iftb::client::getChunkCount() {
    if (!hasFont() or failed)
        return 0;
    return tiftb.getChunkCount();
}

bool iftb::client::setPending(const std::vector<uint32_t> &unicodes,
                              const std::vector<uint32_t> &features) {
    if (!hasFont() or failed)
        return false;
    return tiftb.getMissingChunks(unicodes, features, pendingChunks);
}

bool iftb::client::getPendingChunkList(std::vector<uint16_t> &cl) {
    if (!hasFont() or failed)
        return false;
    cl.clear();
    for (auto i: pendingChunks)
        cl.push_back(i);
    return true;
}

std::pair<uint32_t, uint32_t> iftb::client::getChunkRange(uint16_t cidx) {
    if (!hasFont() or failed)
        std::pair<uint32_t, uint32_t>(0,0);
    return tiftb.getChunkRange(cidx);
}
