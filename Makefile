
DEPDIR := .deps
DEPFLAGS = -MT $@ -MMD -MP -MF ${DEPDIR}/$*.d
CXX := g++
PSRCS := prep.cc config.cc
CHSRCS := builder.cc config.cc main.cc chunk.cc table_IFTC.cc sfnt.cc
SRCS := ${PSRCS} ${CHSRCS}
POBJS := ${PSRCS:.cc=.o}
CHOBJS := ${CHSRCS:.cc=.o}
CFLAGS := -I/home/skef/src/harfbuzz/src -g -std=c++20
CXXFLAGS := ${CFLAGS}
LDFLAGS := -Wl,-rpath /home/skef/src/harfbuzz/build/src -L/home/skef/src/harfbuzz/build/src -lharfbuzz-subset -lharfbuzz -lyaml-cpp

all: prep chunkify

chunkify: ${CHOBJS}
	${CXX} -o $@ ${CHOBJS} ${LDFLAGS}

prep: ${POBJS}
	${CXX} -o $@ ${POBJS} ${LDFLAGS}

%.o : %.cc ${DEPDIR}/%.d | ${DEPDIR}
	${CXX} ${CXXFLAGS} ${DEPFLAGS} -c $<

${DEPDIR}: ; @mkdir -p $@

clean:
	${RM} chunkify prep ${CHOBJS} ${POBJS}

DEPFILES := $(SRCS:%.cc=$(DEPDIR)/%.d)
$(DEPFILES):

include ${wildcard ${DEPFILES}}
