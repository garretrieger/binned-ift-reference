
CLIBASES := chunker config main chunk table_IFTB sfnt sanitize merger cmap client randtest
WASMSRCS := wasm_wrapper.cc client.cc sfnt.cc cmap.cc merger.cc table_IFTB.cc

WOFF2SRCS := woff2_dec.cc variable_length.cc woff2_common.cc woff2_out.cc table_tags.cc
BROTLISRCS := dec/huffman.c dec/bit_reader.c dec/decode.c dec/state.c common/dictionary.c common/transform.c

WOFF2DIR := woff2
BROTLIDIR := woff2/brotli/c
HARFBUZZDIR := harfbuzz

BUILDDIR := build
SRCDIR := src
DEPDIR := ${BUILDDIR}/.deps
DEPFLAGS = -MT $@ -MMD -MP -MF ${DEPDIR}/$*.d
CXX := g++
EMXX := emcc
CLISRCS := ${CLIBASES:%=${SRCDIR}/%.cc}
CLIOBJS := ${CLIBASES:%=${BUILDDIR}/%.o}
# CFLAGS := -I${HARFBUZZDIR}/src -g -fsanitize=address
# CFLAGS := -I${HARFBUZZDIR}/src -I${WOFF2DIR}/include -I/opt/homebrew/include -g
CFLAGS := -I${HARFBUZZDIR}/src -I${WOFF2DIR}/include -g
CXXFLAGS := ${CFLAGS} -std=c++17
EMXXSETS := -s ALLOW_MEMORY_GROWTH=1 -s MALLOC=emmalloc -s MODULARIZE=1 -s EXPORT_ES6=1 -s ENVIRONMENT=web -s EXPORTED_RUNTIME_METHODS='["AsciiToString"]' -s ERROR_ON_UNDEFINED_SYMBOLS=1
EMXXDEFS := -Os --closure 1 ${EMXXSETS}
LIBS := -lharfbuzz-subset -lharfbuzz -lyaml-cpp -lbrotlienc -lwoff2enc -lbrotlidec -lwoff2dec
# LDFLAGS := -Wl,-rpath ${HARFBUZZDIR}/build/src:${WOFF2DIR}/build -L${HARFBUZZDIR}/build/src -L${WOFF2DIR}/build -L/opt/homebrew/lib
LDFLAGS := -Wl,-rpath ${HARFBUZZDIR}/build/src:${WOFF2DIR}/build/ -L${HARFBUZZDIR}/build/src -L${WOFF2DIR}/build

all: iftb iftb.js

iftb: ${CLIOBJS} ${BUILDDIR}
	${CXX} -o $@ ${CLIOBJS} ${LDFLAGS} ${LIBS}

iftb.js: ${WASMSRCS:%=src/%} ${WOFF2SRCS:%=${WOFF2DIR}/src/%} ${BROTLISRCS:%=${BROTLIDIR}}
	${EMXX} ${WASMSRCS:%=src/%} ${WOFF2SRCS:%=${WOFF2DIR}/src/%} ${BROTLISRCS:%=${BROTLIDIR}/%} -I${WOFF2DIR}/include -I${BROTLIDIR}/include -o $@ ${EMXXDEFS} -s -s EXPORTED_FUNCTIONS=@src/iftb.symbols

${BUILDDIR}/%.o : ${SRCDIR}/%.cc ${DEPDIR}/%.d | ${DEPDIR}
	${CXX} ${CXXFLAGS} ${DEPFLAGS} -o $@ -c $<

${DEPDIR}: ; @mkdir -p $@

${BUILDDIR}: ; @mkdir -p $@

clean:
	${RM} iftb ${CLIOBJS} iftb.js iftb.wasm

DEPFILES := $(CLIBASES:%=$(DEPDIR)/%.d)
$(DEPFILES):

include ${wildcard ${DEPFILES}}
