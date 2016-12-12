CC := $(CC)
CXX := $(CXX)
CXXFLAGS := $(CXXFLAGS) -Iinclude -Imason_packages/.link/include -Ideps/geojson-cpp/include -Ideps/wagyu/include -Ideps/vector-tile/include -std=c++14 -Lmason_packages/.link/lib
RELEASE_FLAGS := -O3 -DNDEBUG
WARNING_FLAGS := -Wall -Wextra -Werror -Wsign-compare -Wfloat-equal -Wfloat-conversion -Wshadow -Wno-unsequenced
DEBUG_FLAGS := -g -O0 -DDEBUG -fno-inline-functions -fno-omit-frame-pointer
MASON ?= .mason/mason

default: build/all

clean:
	rm -rf mason_packages
	rm -rf *.dSYM/
	rm -f m2f
	rm -f m2z
	rm -f m2t

$(MASON):
	git submodule update --init

mason_packages: $(MASON)
	$(MASON) install variant 1.1.1 && $(MASON) link variant 1.1.1
	$(MASON) install geometry 0.8.1 && $(MASON) link geometry 0.8.1
	$(MASON) install rapidjson 1.1.0 && $(MASON) link rapidjson 1.1.0
	$(MASON) install protozero 1.4.2 && $(MASON) link protozero 1.4.2
	$(MASON) install sqlite 3.9.1 && $(MASON) link sqlite 3.9.1

build/all: mason_packages
	$(CXX) src/map_to_features.cpp -o m2f -isystem$(MASON_HOME)/include $(CXXFLAGS) $(LDFLAGS)
	$(CXX) src/map_to_zoom.cpp -o m2z -isystem$(MASON_HOME)/include $(CXXFLAGS) $(LDFLAGS)
	$(CXX) src/map_to_tile.cpp -o m2t -isystem$(MASON_HOME)/include $(CXXFLAGS) $(LDFLAGS) $(DEBUG_FLAGS)
	$(CXX) src/reduce_to_mvt.cpp -o r2mvt -isystem$(MASON_HOME)/include -lsqlite3 $(CXXFLAGS) $(LDFLAGS)

test: build/all
	rm -f out.mbtiles
	time cat test/fixtures/countries.geojson | ./m2f foo | grep France | ./m2z --min 1 --max 1 | ./m2t | sort | tee out.geojson | ./r2mvt out.mbtiles
