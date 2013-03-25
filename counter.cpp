#include "counter.hpp"

namespace counter {

#ifndef RELACY
namespace {
__thread intptr_t local_ctr[N_Counter];
std::mutex global_ctr_mu;
intptr_t global_ctr[N_Counter];
}

void finalize() {
	global_ctr_mu.lock();
	for(int i=0;i<N_Counter;i++)
		global_ctr[i] += local_ctr[i];
	global_ctr_mu.unlock();
}

intptr_t get_counter(int c) {
	global_ctr_mu.lock();
	intptr_t r = global_ctr[c];
	global_ctr_mu.unlock();
	return r;
}

void inc_counter(int c, intptr_t inc) {
	local_ctr[c] += inc;
}

void reset() {
	global_ctr_mu.lock();
	for(int i=0;i<N_Counter;i++)
		global_ctr[i] = 0;
	global_ctr_mu.unlock();
}

void local_reset() {
	for(int i=0;i<N_Counter;i++)
		local_ctr[i] = 0;
}
#else
void inc_counter(int, intptr_t) {
}
#endif

}
