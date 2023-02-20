from fontTools.ttLib.tables import DefaultTable
from fontTools.misc import sstruct
import struct

CHUNK_HEADER_FORMAT = """
    > # big endian
    majorVersion:       H
    minorVersion:       H
    checksum:           I
    chunkIndex:         H
    glyphCount:         I
    tableCount:         B
"""

CHUNK_HEADER_SIZE = sstruct.calcsize(CHUNK_HEADER_FORMAT)

class chunk:
    def __init__(self):
        pass

    def decompile(self, data, ttFont = None):
        offset = 0;
        size = CHUNK_HEADER_SIZE
        sstruct.unpack(CHUNK_HEADER_FORMAT, data[0:size], self)
        offset += size

        gidFmt = ">" + "H" * self.glyphCount;
        size = 2 * self.glyphCount
        self.gids = struct.unpack(gidFmt, data[offset:offset + size])
        offset += size

        tableFmt = ">" + "I" * self.tableCount
        size = 4 * self.tableCount
        self.tables = struct.unpack(tableFmt, data[offset:offset + size])
        offset += size

        offsetFmt = ">" + "I" * (self.tableCount * self.glyphCount + 1)
        size = 4 * (self.tableCount * self.glyphCount + 1)
        offsets = struct.unpack(offsetFmt, data[offset:offset + size])
        self.contents = []
        current_content_list = []
        for i in range(self.tableCount * self.glyphCount):
            if i == self.glyphCount:
                self.contents.append(current_content_list)
                current_content_list = []
            current_content_list.append(data[offsets[i]:offsets[i+1]])
        self.contents.append(current_content_list)

        print(vars(self))
        print(len(self.contents))





