
DEPDIR := .deps
DEPFLAGS = -MT $@ -MMD -MP -MF ${DEPDIR}/$*.d
CXX := g++
SRCS := chunker.cc config.cc main.cc chunk.cc table_IFTB.cc sfnt.cc sanitize.cc unchunk.cc cmap.cc
OBJS := ${SRCS:.cc=.o}
CFLAGS := -I/home/skef/src/harfbuzz/src -g
CXXFLAGS := ${CFLAGS} -std=c++17
LIBS := -lharfbuzz-subset -lharfbuzz -lyaml-cpp -lbrotlienc -lwoff2enc -lbrotlidec -lwoff2dec
LDFLAGS := -Wl,-rpath /home/skef/src/harfbuzz/build/src -L/home/skef/src/harfbuzz/build/src

all: iftb

iftb: ${OBJS}
	${CXX} -o $@ ${OBJS} ${LDFLAGS} ${LIBS}

%.o : %.cc ${DEPDIR}/%.d | ${DEPDIR}
	${CXX} ${CXXFLAGS} ${DEPFLAGS} -c $<

${DEPDIR}: ; @mkdir -p $@

clean:
	${RM} iftb ${OBJS}

DEPFILES := $(SRCS:%.cc=$(DEPDIR)/%.d)
$(DEPFILES):

include ${wildcard ${DEPFILES}}
