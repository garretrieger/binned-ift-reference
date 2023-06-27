/*
Copyright 2023 Adobe
All Rights Reserved.

NOTICE: Adobe permits you to use, modify, and distribute this file in
accordance with the terms of the Adobe license agreement accompanying
it.
*/

/* The randtest function produces random combinations of codepoints and
   features and calculates both the IFTB chunk set and the harfbuzz
   subsetter glyph closure for those parameters. It then verifies that
   the set of glyphs corresponding to those chunks is a superset of the
   glyph closure. It is only included in the encoder.
 */
 
#include <string>

#pragma once

namespace iftb {
    bool randtest(std::string &input_string, uint32_t iterations = 10000);
}
