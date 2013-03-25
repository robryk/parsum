#include <unistd.h>
#include "counter.hpp"
#include "bench.hpp"

bool benchmark::should_continue() {
	return !stop_.load(std::memory_order_relaxed);
}

void benchmark::internal_thread(int idx) {
	counter::local_reset();
	while (!start_.load(std::memory_order_relaxed)) {
	}
	thread(idx);
	counter::finalize();
}

void benchmark::run(int seconds, int n_threads) {
	counter::reset();
	counter::local_reset();
	before();
	start_.store(false);
	stop_.store(false);
	std::thread thrs[n_threads];
	for(int i=0;i<n_threads;i++)
		thrs[i] = std::thread(&benchmark::internal_thread, this, i);
	start_.store(true);
	sleep(seconds);
	stop_.store(true);
	for(int i=0;i<n_threads;i++)
		thrs[i].join();
	after();
	counter::finalize();
}

