/*
Copyright 2023 Adobe
All Rights Reserved.

NOTICE: Adobe permits you to use, modify, and distribute this file in
accordance with the terms of the Adobe license agreement accompanying
it.
*/

import Module from './iftb.js'

const iftb = await Module()

export class iftb_font {
    constructor(ranges = false, verbose = false) {
        this.cl = iftb._iftb_new_client();
        this.ranges = !!ranges;
        this.verbose = !!verbose;
        if (this.verbose)
            console.log('Constructed new iftb_font object ' + this.cl);
    }

    async initialize(url) {
        if (this.verbose)
            console.log('Initializing iftb_font object ' + this.cl + ' with ' + url);
        let response = await fetch(url);
        let data = await response.arrayBuffer();
        let loc = iftb._iftb_reserve_initial_font_data(this.cl, data.byteLength);
        let arr = new Uint8Array(data);
        iftb.HEAPU8.set(arr, loc);
        if (!iftb._iftb_decode_initial_font(this.cl) ) {
            console.log('Font loading failed');
            return false;
        }
        this.orig_url = url;
        this.chunk_count = iftb._iftb_get_chunk_count(this.cl);
        if (this.ranges) {
            let sptr = iftb._iftb_range_file_uri(this.cl);
            if (sptr == 0) {
                console.log('Problem retriving range file URI');
                return false;
            }
            this.range_url = new URL(iftb.AsciiToString(sptr), url);
            this.range_length = iftb._iftb_get_chunk_offset(this.cl, this.chunk_count);
        }
        return true;
    }

    chunks_for_augmentation(codepoints, features) {
        let cnum = codepoints.size;
        let cptr = iftb._iftb_reserve_unicode_list(this.cl, cnum);
        if (cnum > 0) {
            let carr = Uint32Array.from(codepoints.values());
            iftb.HEAPU32.set(carr, cptr/4);
        }
        let fnum = features.size;
        let fptr = iftb._iftb_reserve_feature_list(this.cl, fnum);
        if (fnum > 0) {
            let farr = Uint32Array.from(features.values());
            iftb.HEAPU32.set(farr, fptr/4);
        }
        if (cnum > 0 || fnum > 0) {
            if (!iftb._iftb_compute_pending(this.cl)) {
                console.log('Problem computing pending chunks');
                return [];
            }
        }
        cnum = iftb._iftb_get_pending_list_count(this.cl);
        cptr = iftb._iftb_get_pending_list_location(this.cl);
        if (this.verbose)
            console.log("Additional chunk count is " + cnum);
        return Array.from(iftb.HEAPU16.subarray(cptr/2, cptr/2+cnum))
    }

    get_chunk_file_uri(cidx) {
        let sptr = iftb._iftb_chunk_file_uri(this.cl, cidx);
        if (sptr == 0) {
            console.log('Problem retriving chunk file URI');
            return '';
        }
        let r = iftb.AsciiToString(sptr);
        if (this.verbose)
            console.log('Chunk ' + cidx + ' URI is ' + r);
        return r;
    }

    add_chunk(cidx, data, force) {
        let dptr = iftb._iftb_reserve_chunk_data(this.cl, cidx,
                                                 data.byteLength);
        if (dptr == 0) {
            console.log('Problem reserving chunk data');
            return false;
        } else if (dptr == 1) {
            if (this.verbose) {
                console.log('Not adding redundant chunk ' + cidx);
            }
            return true;
        }
        iftb.HEAPU8.set(data, dptr);
        let ok = iftb._iftb_use_chunk_data(this.cl, cidx, force);
        if (!ok) {
            console.log('Problem loading or processing chunk data');
            return false;
        }
        return true;
    }

    can_merge() {
        return !!iftb._iftb_can_merge(this.cl);
    }

    merge(for_cache) {
        let ok = !!iftb._iftb_merge(this.cl, for_cache);
        if (!ok) {
            console.log('Could not merge chunks');
            return false;
        }
        return true;
    }

    async get_url_data(url) {
        let response = await fetch(new URL(url, this.orig_url));
        return await response.arrayBuffer();
    }

    async chunks_from_files(chunklist) {
        let promiselist = [];
        for (const cidx of chunklist) {
            let cs = this.get_chunk_file_uri(cidx);
            if (cs === '') {
                console.log('Cannot get chunk ' + cidx);
                continue;
            }
            promiselist.push(this.get_url_data(cs));
        }
        let datalist = await Promise.all(promiselist);
        let i = 0;
        let chunkdata = [];
        for (const cidx of chunklist) {
            chunkdata.push([cidx, new Uint8Array(datalist[i++])]);
        }
        return chunkdata;
    }

    /* --- ranges ---
     *
     * "chunkranges" is an array of 3-tuples, where the first element
     * is a chunk index, the second is the offset of the first byte in
     * the range file, and the third is the offset of the last byte
     * in the range file (inclusive).
     *
     * "reqranges" is an array of ascii "ranges" matching /[0-9]+-[0-9]+]
     * These are eventually used in the "Range" header of a http
     * range-request.
     *
     * A "rangelist" is an array of 3-tuples, where the first element
     * is a Uint8Array that is a copy of a portion of the range file, the
     * second element is the offset in the range file of the first byte of
     * that portion, and the third element is the offset in the range file
     * of the last byte of the portion (inclusive). These are distilled
     * from responses to http range requests.
     */

    /* When the response to a range request returns the whole file (perhaps
     * because the server doesn't support the range-request protocol) the
     * client should extract and load all of the chunks from it. This method
     * returns a chunkranges array for all chunks (except 0).
     */
    all_chunk_ranges() {
        let startbyte = 0;
        let chunkranges = [];
        for (let i = 1; i < this.chunk_count; i++) {
            let nextstartbyte = iftb._iftb_get_chunk_offset(this.cl, i+1);
            chunkranges.push([i, startbyte, nextstartbyte - 1]);
            startbyte = nextstartbyte;
        }
        return chunkranges;
    }

    /* Returns the chunkranges and reqranges corresponding to a list of chunk
     * indexes.
     */
    get_ranges(chunklist) {
        let chunkranges = [];
        let reqranges = [];
        let lastcidx = -1;
        let startrange = 0;
        let startbyte = 0;
        let lastendbyte = 0;
        for (const cidx of chunklist) {
            let endbyte = iftb._iftb_get_chunk_offset(this.cl, cidx + 1) - 1;
            if (lastcidx + 1 == cidx) {
                startbyte = lastendbyte + 1;
            } else {
                if (lastcidx != -1) {
                    reqranges.push(startrange.toString() + '-' + lastendbyte.toString());
                }
                startrange = startbyte = iftb._iftb_get_chunk_offset(this.cl, cidx);
            }
            chunkranges.push([cidx, startbyte, endbyte]);
            lastcidx = cidx;
            lastendbyte = endbyte;
        }
        reqranges.push(startrange.toString() + '-' + lastendbyte.toString());
        return [chunkranges, reqranges];
    }

    parse_content_range(s) {
        let first = 0, last = 0;
        let m = /bytes ([0-9]+)-([0-9]+)\//i.exec(s);
        if (m !== null && m.length == 3) {
            first = m[1];
            last = m[2];
        }
        return [first, last];
    }

    /* The multipart/byteranges format is encoded as sequence of range
     * descriptions, each consisting of a boundary string and a set of
     * ascii headers followed by binary data.
     *
     * Because chunks extracted from the range file are quite long it
     * is best to avoid seaching for the boundary string within chunk
     * data. Instead we look for the boundary marker and then the sequence
     * of four bytes matching crlfcrlf that terminates the headers, which
     * will be followed by the binary data. We then turn the header portion
     * of the description into a string and extract the Content-Range
     * value, which tells us the start and end bytes encoded, and therefore
     * the length of the binary data. That also gets us the start of the
     * next header (or the terminating boundary string). The header portion
     * of each description is short enough that a non-optimized search for
     * the terminating crlfcrlf is fine.
     *
     * The boundary string is supplied in the content-type field of the
     * response headers. Each range description starts with two hyphens
     * and then the boundary string followed by crlf. The file is terminated
     * by two hyphens, the boundary string, and another two hyphens.
     */
    parse_multipart(boundary, body) {
        let u8body = new Uint8Array(body);
        let asciidec = new TextDecoder('ascii');
        let crexp = new RegExp('^Content-Range: bytes ([0-9]+)-([0-9]+)/', 'mi');
        let hstart = 0;
        let rangelist = [];
        while (true) {
            let hend = hstart;
            while (hend < hstart + 20 && u8body[hend] != 0x2D)  // Hyphen
                hend++;
            if (u8body[hend] === undefined) {
                console.log("Error: Cannot find boundary after " +
                            rangelist.length.toString() + " parts.");
                return [];
            } else if (hend >= hstart + 20) {
                console.log("Error: Cannot find multipart boundary start " +
                            "within 20 bytes.");
                return [];
            }
            let hb = '--' + boundary;
            if (asciidec.decode(u8body.slice(hend, hend + hb.length)) !== hb) {
                console.log("Error: Boundary not at start of multipart/byteranges " +
                            "range.");
                return [];
            }
            hend += hb.length;
            if (u8body[hend] === 0x2D && u8body[hend + 1] === 0x2D)
                return rangelist;
            else if (u8body[hend] != 0xD) {  // Carriage return
                console.log("Error: Badly formed multipart boundary.");
                return [];
            }
            while (hend < hstart + 2000 && !(u8body[hend] === 0xD &&
                                             u8body[hend+1] === 0xA &&
                                             u8body[hend+2] === 0xD &&
                                             u8body[hend+3] === 0xA)) {
                hend++;
            }
            if (hend >= hstart + 2000) {
                console.log("Error: multipart/byteranges headers malformed or too long.");
                return [];
            }
            hend += 4;
            let rheaders = asciidec.decode(u8body.slice(hstart, hend));
            let m = crexp.exec(rheaders);
            if (m === null || m.length < 3) {
                console.log("Error: multipart/byteranges range headers missing " +
                            "Content-Range");
                return [];
            }
            let rstart = parseInt(m[1]), rend = parseInt(m[2]);
            let l = rend - rstart + 1;
            rangelist.push([new Uint8Array(body, hend, l), rstart, rend]);
            hstart = hend + l;
        }
        return undefined;  // Should never be reached
    }

    /*
     * Note that the server has a lot of freedom in how it responds to
     * range requests. If it does not support the protocol at all it will
     * send the whole file. It can also decide to combine ranges, which
     * means it may return fewer descriptions than requested. It can also
     * reorder the ranges returned, although the IFTB specification recommends
     * always asking for ranges in order and avoiding requests with gapless
     * ranges. Technically the specification allows the server to turn in-order
     * range requests into out-of-order range responses, but we assume that
     * won't happen in practice.
     *
     * While a multipart/byteranges file with a single range is valid, a
     * server will typically respond to a request for one range with a 206
     * status that includes a content-range record in the top-level headers
     * and a pure binary body. It may also respond this way to a request
     * for multiple ranges if it decides to combine them. A client must
     * therefore be prepared to match up requests with responses in a variety
     * of ways.
     */
    async chunks_from_range_file(chunklist) {
        let force = false;
        let [chunkranges, reqranges] = this.get_ranges(chunklist);
        let lb = this.range_length - 1;
        let getting_all = (reqranges.length == 1 &&
                           reqranges[0] == ('0-' + lb.toString()));
        let req_headers = {};
        if (!getting_all) {
//            req_headers['Content-Type'] = 'multipart/byteranges';
            req_headers['Range'] = 'bytes=' + reqranges.join(',');
        }
        let response = await fetch(this.range_url, { headers: req_headers });
        let body = await response.arrayBuffer();
        let rangelist = [];
        let ct = response.headers.get('content-type');
        if (response.status == 206) {
            if (response.headers.has('content-range')) {
                let cr = response.headers.get('content-range');
                let [first, last] = this.parse_content_range(cr);
                if (last == 0) {
                    console.log("Unrecognized content-range header '" + cr + "'");
                    return [[], false];
                }
                rangelist.push([new Uint8Array(body), parseInt(first), parseInt(last)]);
            } else if (/multipart\/byteranges/i.test(ct)) {
                let m = /boundary=(\w+)/i.exec(ct);
                if (m === null || m.length < 2) {
                    console.log("Server returned multipart/byteranges " +
                                "content without boundary");
                    return [[], false];
                }
                let boundary = m[1];
                rangelist = this.parse_multipart(boundary, body);
            } else {
                console.log("Error: 206 partial response without Content-Range " +
                            "header or multipart/byteranges content.");
                return [[], false];
            }
        } else if (response.status == 200) {
            let len = parseInt(response.headers.get('content-length'));
            if (len !== this.range_length) {
                console.log("Error: whole file returned, expected length " +
                            this.range_length.toString() + ", got length " +
                            len.toString());
                return [[], false];
            }
            if (!getting_all) {
                chunkranges = this.all_chunk_ranges();
                force = true;
            }
            rangelist.push([new Uint8Array(body), 0, len-1]);
        } else {
            console.log("Bad response status value " + response.status.toString());
            return [[], false];
        }
        // Match up the chunkranges with the extracted data.
        let chunkdata = [];
        let ri = 0;
        let r = rangelist[ri];
        for (const cr of chunkranges) {
            while (cr[1] >= r[2])
                r = rangelist[++ri];
            if (r === undefined || cr[1] < r[1] || cr[2] > r[2]) {
                console.log("Could not find chunk " + cr[0].toString() +
                            " among ranges.");
                return [];
            }
            chunkdata.push([cr[0], new Uint8Array(body,
                                                  r[0].byteOffset + cr[1] - r[1],
                                                  cr[2] - cr[1] + 1)]);
        }
        return [chunkdata, force];
    }


    async augment(codepoints, features) {
        if (this.verbose) {
            console.log('Codepoints: ' + Array.from(codepoints).join(','));
        }
        let chunklist = this.chunks_for_augmentation(codepoints, features);
        if (chunklist.length == 0) {
            if (this.verbose)
                console.log('Augmenting iftb_font object ' + this.cl +
                            ': No additional chunks needed');
            return true;
        }
        if (this.verbose)
            console.log('Augmenting iftb_font object ' + this.cl +
                        ' with chunks [' + chunklist.join(', ') + ']');
        let chunkdata;
        let force = false;
        if (this.ranges) {
            [chunkdata, force] = await this.chunks_from_range_file(chunklist);
        } else {
            chunkdata = await this.chunks_from_files(chunklist);
        }
        if (chunkdata.length == 0) {
            console.log('Failed to retrieve chunks for augmentation');
            return false;
        }
        for (const [cidx, data] of chunkdata) {
            if (this.verbose)
                console.log('Adding chunk ' + cidx + ', length ' +
                            data.byteLength);
            if (!this.add_chunk(cidx, data, force))
                return;
        }
        return this.merge(false);
    }

    async load(family, args) {
        if (this.verbose)
            console.log('(Re?)-loading iftb_font object ' + this.cl);
        let flen = iftb._iftb_get_font_length(this.cl);
        let floc = iftb._iftb_get_font_location(this.cl, 0);
        const font = new FontFace(family, iftb.HEAPU8.subarray(floc, floc+flen), args);
        document.fonts.add(font);
        await font.load();
    }

    async initialize_load(url, family, args) {
        await this.initialize(url);
        await this.load(family, args);
    }

    async augment_load(codepoints, features, family, args) {
        await this.augment(codepoints, features);
        await this.load(family, args);
    }

    async init_augment_load(url, codepoints, features, family, args) {
        await this.initialize(url);
        await this.augment(codepoints, features);
        await this.load(family, args);
    }
};

export default iftb_font;
