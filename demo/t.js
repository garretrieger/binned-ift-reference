/*
Copyright 2023 Adobe
All Rights Reserved.

NOTICE: Adobe permits you to use, modify, and distribute this file in
accordance with the terms of the Adobe license agreement accompanying
it.
*/

import iftb_font from './iftb_font.js'

let f = new iftb_font(false, true);

let start = Date.now();

let text = document.body.innerText;
let featset = new Set();
let cpset = new Set();

for(let i = 0; text.codePointAt(i); i++)
    cpset.add(text.codePointAt(i));

let font_url = new URL('./fonts/NotoSerifCJKjp-Regular_iftbJA.woff2',
                       document.URL);

// f.initialize_load(font_url, "B Font", { style: "normal", weight: "400" });
//
f.init_augment_load(font_url, cpset, featset, "B Font",
                    { style: "normal", weight: "400" })
    .then(() => {
        console.log('Milliseconds elapsed in t.js: ' + (Date.now() - start));
    });
