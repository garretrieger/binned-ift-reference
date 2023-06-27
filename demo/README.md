# Summary

This is a *very* early demo of the IFTB chunked font technology, primarily
useful as live code that interacts with the IFTB WASM library.

Note that most simple loopback HTTP server implementations do not support range-
request. The `nginx_custom.conf` file is included as a starting point for users
who want to try out the rangefile option. It must be modified to point to the right
directories and run as `nginx -c nginx_custom.conf`. 

Also note that most loopback HTTP server implementations are HTTP/1.x based, and
therefore do not provide a good indication of performance when loading many chunk
files at once. HTTP/2.x is much better for that use case.

# Local Use

1. On the command line `cd` into this directory.
3. Run a local HTTP server to provide the content, e.g. `python -m http.server`
4. Direct your browser to the page served by the HTTP server, e.g. `http://localhost:8000/`

You can then currently choose between pages `t.html` and `u.html`.

It can be helpful to open developer "console" in the browser to view status
messages from the "verbose" mode of the demo javascript client, which are turned on
by default.  E.g. on Chrome open "More Tools", "Developer Tools" and pick the
"Console" tab.

# Other Use

The browser interactions make no special use of the server—all content is in
the form of normal files, some pre-compressed.  Therefore you can also stage
the files in the demo directory any other HTTP service.  However, you cannot
just load the files directly using a `file://` URL due to restrictions on WASM
access to local files
