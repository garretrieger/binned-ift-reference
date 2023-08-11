import iftb_font from './iftb_font.js'

let page_index = -1;
let states = {};
let also_load_unicode_range = true;
let show_unicode_range = false;

async function update_all_fonts() {
    if (page_index < 0) {
        update_transfer_bars();
        return;
    }

    let title_font = SENTENCES[page_index][0];
    let title_text = SENTENCES[page_index][1];
    let paragraph_font = PARAGRAPHS[page_index][0];
    let paragraph_text = PARAGRAPHS[page_index][1];

    let p1 = update_fonts(title_text,
                          title_font,
                          "Title Font");
    let p2 = update_fonts(paragraph_text,
                          paragraph_font,
                          "Paragraph Font");
    await p1;
    await p2;

    document.getElementById("title_pfe").textContent = title_text;
    document.getElementById("paragraph_pfe").textContent = paragraph_text;
    if (also_load_unicode_range) {
        document.getElementById("title_ur").textContent = title_text;
        document.getElementById("paragraph_ur").textContent = paragraph_text;
    }

    document.getElementById("prev").disabled = (page_index == 0);
    document.getElementById("next").disabled = (page_index == PARAGRAPHS.length - 1);

    update_transfer_bars();
    update_sample_toggle();
}

function update_sample_toggle() {
    document.getElementById("sample_toggle").style.visibility =
        (page_index >= 0 && also_load_unicode_range) ? "visible" : "hidden";
    document.getElementById("sample_toggle").value =
        show_unicode_range ? "Show progressive font enrichment" : "Show unicode range";

    if (show_unicode_range) {
        document.getElementById("pfe_sample").classList.add("hide");
        document.getElementById("ur_sample").classList.remove("hide");
    } else {
        document.getElementById("pfe_sample").classList.remove("hide");
        document.getElementById("ur_sample").classList.add("hide");
    }
}

function update_fonts(text, font_id, font_face) {
    let cps = new Set();
    for (let i = 0; text.codePointAt(i); i++) {
        cps.add(text.codePointAt(i));
    }

    return patch_codepoints(font_id, font_face, cps);
}

function patch_codepoints(font_id, font_face, cps) {
    let font_url = new URL(font_id, document.URL);

    let do_init = false;
    if (!states[font_id]) {
      states[font_id] = new iftb_font(false, true);
      do_init = true;      
    }

    let featset = new Set();

    let start = Date.now();
    if (do_init) {
      return states[font_id].init_augment_load(font_url, cps, featset, font_face,
            { style: "normal", weight: "400" })
            .then(() => {
                console.log('Init Load - milliseconds elapsed in t.js: ' + (Date.now() - start));
            });
    }

    return states[font_id].augment_load(cps, featset, font_face,
                       { style: "normal", weight: "400" })
       .then(() => {
           console.log('Augment Load - milliseconds elapsed in t.js: ' + (Date.now() - start));
       });
}

function update_transfer_bars() {
    let pfe_total = 0;
    let ur_total = 0;
    for (let r of performance.getEntriesByType("resource")) {
        if (r.name.includes("fonts/Noto")
            && (r.name.endsWith(".ttf") || r.name.endsWith(".otf") || r.name.endsWith(".woff2")
	       || r.name.endsWith(".br"))) {
            pfe_total += r.transferSize;
        }
        if (r.name.includes("/s/")) {
            ur_total += r.transferSize;
        }
    }
    let total = Math.max(Math.max(pfe_total, ur_total), 1);
    document.getElementById("pfe_bar").style.width =
        ((pfe_total / total) * 100) + "%";
    document.getElementById("pfe_bar").textContent = as_string(pfe_total);

    document.getElementById("ur_bar").style.width =
        ((ur_total / total) * 100) + "%";
    document.getElementById("ur_bar").textContent = as_string(ur_total);

    document.getElementById("ur_byte_counter").style.visibility =
        (also_load_unicode_range ? "visible" : "hidden");
}

function as_string(byte_count) {
    return Math.round(byte_count / 1000).toLocaleString() + " kb";
}

    let prev = document.getElementById("prev");
    prev.addEventListener("click", function() {
        page_index--;
        if (page_index < 0) page_index = 0;
        update_all_fonts();
    });
    let next = document.getElementById("next");
    next.addEventListener("click", function() {
        page_index++;
        if (page_index >= PARAGRAPHS.length) page_index = PARAGRAPHS.length - 1;
        update_all_fonts();
    });
    document.getElementById("also_ur").addEventListener("change", function(e) {
        also_load_unicode_range = e.target.checked;
        if (!also_load_unicode_range) {
            sample_toggle = false;
        }
        update_all_fonts();
        update_sample_toggle();
    });
    document.getElementById("sample_toggle").addEventListener("click", e => {
        show_unicode_range = !show_unicode_range;
        update_sample_toggle();
    });

document.fonts.addEventListener('loadingdone', function() {
    update_transfer_bars();
});
