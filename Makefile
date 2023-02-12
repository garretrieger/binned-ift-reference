
CXX := g++
SRCS := builder.cc config.cc main.cc
OBJS := ${SRCS:.cc=.o}
CFLAGS := -I/home/skef/src/harfbuzz/src -g
CXXFLAGS := ${CFLAGS}
LDFLAGS := -Wl,-rpath /home/skef/src/harfbuzz/build/src -L/home/skef/src/harfbuzz/build/src -lharfbuzz-subset -lharfbuzz -lyaml-cpp

chunkify: ${OBJS}
	${CXX} -o $@ ${OBJS} ${LDFLAGS}

%.o: %.cc
	${CXX} ${CXXFLAGS} -c $<

clean:
	${RM} chunkify ${OBJS}
