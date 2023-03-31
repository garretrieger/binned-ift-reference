#include <sstream>
#include <iostream>

#include "sanitize.h"
#include "sfnt.h"
#include "tag.h"
#include "table_IFTB.h"

bool iftb::sanitize(std::string &s, iftb::config &conf) {
    iftb::sfnt sfnt(s);
    simplestream ss;

    if (!sfnt.read()) {
        return false;
    }

    sfnt.checkSums(conf.verbosity() > 1);

    if (!sfnt.getTableStream(ss, T_IFTB)) {
        std::cerr << "Error: No IFTB table in font file" << std::endl;
        return false;
    }

    table_IFTB tiftb;
    
    if (!tiftb.decompile(ss))
        return false;

    bool is_cff = false, is_variable = false;
    // Determine font type
    if (sfnt.has(T_CFF)) {
        if (sfnt.has(T_CFF2)) {
            std::cerr << "Error: Font has both CFF and CFF2 tables" << std::endl;
            return false;
        } else if (sfnt.has(T_GLYF)) {
            std::cerr << "Error: Font has both CFF and glyf tables" << std::endl;
            return false;
        }
        is_cff = true;
    } else if (sfnt.has(T_CFF2)) {
        if (sfnt.has(T_GLYF)) {
            std::cerr << "Error: Font has both CFF2 and glyf tables" << std::endl;
            return false;
        }
        is_cff = is_variable = true;
    } else if (sfnt.has(T_GLYF)) {
        if (sfnt.has(T_GVAR)) {
            is_variable = true;
        }
    } else
        std::cerr << "Error: No CFF, CFF2 or glyf table in font." << std::endl;

    if (!sfnt.getTableStream(ss, tag("head"))) {
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

void iftb::info(std::string &s, iftb::config &conf) {
    iftb::sfnt sfnt(s);
    simplestream ss;

    if (!sfnt.read()) {
        return;
    }

    if (!sfnt.getTableStream(ss, T_IFTB)) {
        std::cerr << "Error: No IFTB table in font file" << std::endl;
        return;
    }

    table_IFTB tiftb;
    
    if (!tiftb.decompile(ss))
        return;

    if (conf.verbosity() > 2)
        sfnt.dump(std::cerr);

    tiftb.dump(std::cerr, conf.verbosity() > 1);
}
