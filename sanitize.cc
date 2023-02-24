#include <sstream>
#include <iostream>

#include "sanitize.h"
#include "sfnt.h"
#include "tag.h"
#include "table_IFTC.h"

bool iftc_sanitize(std::string &s, bool verbose) {
    sfnt sf(s);
    simplestream ss;

    try {
        sf.read();
    } catch (const std::runtime_error &ex) {
        std::cerr << "Error parsing sfnt headers: " << ex.what() << std::endl;
        return false;
    }

    if (!sf.getTableStream(ss, tag("IFTC"))) {
        std::cerr << "Error: No IFTC table in font file" << std::endl;
        return false;
    }

    table_IFTC tiftc;
    
    try {
        tiftc.decompile(ss);
    } catch (const std::runtime_error &ex) {
        std::cerr << "Error parsing IFTC table: " << ex.what() << std::endl;
        return false;
    }

    if (verbose)
        tiftc.dump(std::cerr);

    bool is_cff = false, is_variable = false;
    // Determine font type
    if (sf.has(T_CFF)) {
        if (sf.has(T_CFF2)) {
            std::cerr << "Error: Font has both CFF and CFF2 tables" << std::endl;
            return false;
        } else if (sf.has(T_GLYF)) {
            std::cerr << "Error: Font has both CFF and glyf tables" << std::endl;
            return false;
        }
        is_cff = true;
    } else if (sf.has(T_CFF2)) {
        if (sf.has(T_GLYF)) {
            std::cerr << "Error: Font has both CFF2 and glyf tables" << std::endl;
            return false;
        }
        is_cff = is_variable = true;
    } else if (sf.has(T_GLYF)) {
        if (sf.has(T_GVAR)) {
            is_variable = true;
        }
    } else
        std::cerr << "Error: No CFF, CFF2 or glyf table in font." << std::endl;

    if (!sf.getTableStream(ss, tag("head"))) {
        std::cerr << "Error: No head table in font file" << std::endl;
        return false;
    }

    if (!is_cff) {
        ss.seekg(50);
        int16_t i2lf = readObject<int16_t>(ss);
        if (i2lf != 1) {
            std::cerr << "Error: indexToLocFormat in head table != 1" << std::endl;
            return false;
        }
    }

    return true;
}
