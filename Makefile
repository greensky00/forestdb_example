LDFLAGS = -pthread -lsnappy -ldl
CFLAGS = \
	-g -D_GNU_SOURCE -std=c++11 \
	-I.

CFLAGS += -Wall
CXXFLAGS = $(CFLAGS)
#CFLAGS += -O3

EXAMPLE = \
	fdb_example.o \
	forestdb/build/libforestdb.a \

PROGRAMS = \
	fdb_example \

all: $(PROGRAMS)

fdb_example: $(EXAMPLE)
	$(CXX) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -rf $(PROGRAMS) ./*.o ./*.so ./*/*.o ./*/*.so