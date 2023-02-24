
#include "chunk.h"
#include "streamhelp.h"

void chunk::compile(std::ostream &os, uint16_t idx,
                    uint32_t table1, uint32_t offset1,
                    blob &blob1, std::vector<glyphrec> &recs1,
                    uint32_t table2, uint32_t offset2,
                    blob &blob2, std::vector<glyphrec> &recs2,
                    uint32_t offset) {
    unsigned int length1, length2;
    bool twotables = (table2 != 0);
    const char *base1 = hb_blob_get_data(blob1.b, &length1);
    if (twotables)
        const char *base2 = hb_blob_get_data(blob2.b, &length2);
    os.seekp(offset);
    writeObject(os, (uint16_t) 0);
    writeObject(os, (uint16_t) 1);
    writeObject(os, (uint32_t) 0);
    writeObject(os, idx);
    writeObject(os, (uint32_t) gids.size());
    writeObject(os, (uint8_t) (twotables ? 2 : 1));
    uint32_t gid = HB_SET_VALUE_INVALID;
    while (gids.next(gid))
        writeObject(os, (uint16_t) gid);
    writeObject(os, table1);
    uint32_t c_offset = sizeof(uint32_t) * gids.size();
    if (twotables) {
        writeObject(os, table2);
        c_offset *= 2;
    }
    c_offset += sizeof(uint32_t);  // Last offset encodes length
    c_offset += os.tellp();
    gid = HB_SET_VALUE_INVALID;
    while (gids.next(gid)) {
        writeObject(os, c_offset);
        c_offset += recs1[gid].length;
    }
    if (twotables) {
        gid = HB_SET_VALUE_INVALID;
        while (gids.next(gid)) {
            writeObject(os, c_offset);
            c_offset += recs2[gid].length;
        }
    }
    writeObject(os, c_offset);
    gid = HB_SET_VALUE_INVALID;
    while (gids.next(gid))
        os.write(base1 + offset1 + recs1[gid].offset, recs1[gid].length);
    if (twotables) {
        gid = HB_SET_VALUE_INVALID;
        while (gids.next(gid))
            os.write(base1 + offset1 + recs2[gid].offset, recs2[gid].length);
    }
}
