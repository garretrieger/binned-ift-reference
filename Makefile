
DEPDIR := .deps
DEPFLAGS = -MT $@ -MMD -MP -MF ${DEPDIR}/$*.d
CXX := g++
BDSRCS := builder.cc config.cc main.cc chunk.cc table_IFTC.cc
CLSRCS := cmain.cc merger.cc table_IFTC.cc sfnt.cc sanitize.cc
BDOBJS := ${BDSRCS:.cc=.o}
CLOBJS := ${CLSRCS:.cc=.o}
CFLAGS := -I/home/skef/src/harfbuzz/src -g -std=c++17
CXXFLAGS := ${CFLAGS}
BDLIBS := -lharfbuzz-subset -lharfbuzz -lyaml-cpp -lbrotlienc -lwoff2enc
CLLIBS := -lbrotlidec -lwoff2dec
BDLDFLAGS := -Wl,-rpath /home/skef/src/harfbuzz/build/src -L/home/skef/src/harfbuzz/build/src

all: chunkify ccmd

chunkify: ${BDOBJS}
	${CXX} -o $@ ${BDOBJS} ${BDLDFLAGS} ${BDLIBS}

ccmd: ${CLOBJS}
	${CXX} -o $@ ${CLOBJS} ${CLLIBS}

%.o : %.cc ${DEPDIR}/%.d | ${DEPDIR}
	${CXX} ${CXXFLAGS} ${DEPFLAGS} -c $<

${DEPDIR}: ; @mkdir -p $@

clean:
	${RM} chunkify ccmd ${BDOBJS} ${CLOBJS}

DEPFILES := $(SRCS:%.cc=$(DEPDIR)/%.d)
$(DEPFILES):

include ${wildcard ${DEPFILES}}
