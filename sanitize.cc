#include <sstream>
#include <iostream>

#include "sanitize.h"
#include "sfnt.h"
#include "tag.h"
#include "table_IFTC.h"

bool iftc_sanitize(std::string &s, bool verbose) {
    sfnt sf(s);
    uint32_t l;

    try {
        sf.read();
    } catch (const std::runtime_error &ex) {
        std::cerr << "Error parsing sfnt headers: " << ex.what() << std::endl;
        return false;
    }

    if (!sf.has(tag("IFTC"))) {
        std::cerr << "No IFTC table in file.";
        return false;
    }

    table_IFTC tiftc;
    
    try {
        tiftc.decompile(sf.getTableStream(tag("IFTC"), l));
    } catch (const std::runtime_error &ex) {
        std::cerr << "Error parsing IFTC table: " << ex.what() << std::endl;
        return false;
    }

    if (verbose)
        tiftc.dump(std::cerr);

    return true;
}
