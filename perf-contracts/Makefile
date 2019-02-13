CXXFLAGS = -std=c++11 -Wall -g -I$(KLEE_INCLUDE)
# Flag for contracts with alternate chain data structure
#CXXFLAGS += -DALT_CHAIN
#CXXFLAGS += -DREHASHING_MAP
LDLIBS = -ldl

default: perf-contracts.so test-dl

%.so: %.cpp
	g++ $(CXXFLAGS) -fPIC -shared $< -o $@

%: %.cpp
	g++ $(CXXFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f *.so test-dl
