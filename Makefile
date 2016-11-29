CC := $(CC)
CXX := $(CXX)
CXXFLAGS := $(CXXFLAGS) -Iinclude -Imason_packages/.link/include -Ideps/geojson-cpp/include -std=c++14
RELEASE_FLAGS := -O3 -DNDEBUG
WARNING_FLAGS := -Wall -Wextra -Werror -Wsign-compare -Wfloat-equal -Wfloat-conversion -Wshadow -Wno-unsequenced
DEBUG_FLAGS := -g -O0 -DDEBUG -fno-inline-functions -fno-omit-frame-pointer
MASON ?= .mason/mason

default: build/all

clean:
	rm -rf mason_packages

$(MASON):
	git submodule update --init

mason_packages: $(MASON)
	$(MASON) install variant 1.1.1 && $(MASON) link variant 1.1.1
	$(MASON) install wagyu 0.1.0 && $(MASON) link wagyu 0.1.0
	$(MASON) install geometry 0.8.1 && $(MASON) link geometry 0.8.1
	$(MASON) install rapidjson 1.1.0 && $(MASON) link rapidjson 1.1.0

build/all: mason_packages
	$(CXX) src/map_to_features.cpp -o m2f -isystem$(MASON_HOME)/include $(CXXFLAGS) $(LDFLAGS)
	$(CXX) src/map_to_zoom.cpp -o m2z -isystem$(MASON_HOME)/include $(CXXFLAGS) $(LDFLAGS)

test: build/all
	cat test/fixtures/countries.geojson | ./m2f foo | ./m2z --min 0 --max 14 > out.geojson
