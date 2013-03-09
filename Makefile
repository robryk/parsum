CXXFLAGS=-std=c++0x -g -march=native -pthread -O2
CXXFLAGS_RELACY=-std=c++0x -I../relacy -g -march=native -O2 -DRELACY
LDFLAGS=-pthread
CC=g++
CXX=g++

%.relacy.o: %.cpp
		$(CXX) $(CXXFLAGS_RELACY) -c $< -o $@

history_bench: history.o history_test.o

history_rrd_test: history.relacy.o history_test.relacy.o
	$(CXX) $^ -o $@
holder_rrd_test: holder.relacy.o holder_test.relacy.o
	$(CXX) $^ -o $@
parsum_rrd_test: history.relacy.o holder.relacy.o parsum_test.relacy.o 
	$(CXX) $^ -o $@
