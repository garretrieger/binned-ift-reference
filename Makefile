
DEPDIR := .deps
DEPFLAGS = -MT $@ -MMD -MP -MF ${DEPDIR}/$*.d
CXX := g++
SRCS := builder.cc config.cc main.cc
OBJS := ${SRCS:.cc=.o}
CFLAGS := -I/home/skef/src/harfbuzz/src -g
CXXFLAGS := ${CFLAGS}
LDFLAGS := -Wl,-rpath /home/skef/src/harfbuzz/build/src -L/home/skef/src/harfbuzz/build/src -lharfbuzz-subset -lharfbuzz -lyaml-cpp

chunkify: ${OBJS}
	${CXX} -o $@ ${OBJS} ${LDFLAGS}

%.o : %.cc ${DEPDIR}/%.d | ${DEPDIR}
	${CXX} ${CXXFLAGS} ${DEPFLAGS} -c $<

${DEPDIR}: ; @mkdir -p $@

clean:
	${RM} chunkify ${OBJS}

DEPFILES := $(SRCS:%.cc=$(DEPDIR)/%.d)
$(DEPFILES):

include ${wildcard ${DEPFILES}}
