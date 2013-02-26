CXXFLAGS=-std=c++0x -I../relacy -DRELACY -g
CC=g++

history_test: history.o history_test.o
holder_test: holder.o holder_test.o
parsum_test: history.o holder.o parsum_test.o 
