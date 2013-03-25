#include "parsum.hpp"
#include "counter.hpp"
#include "bench.hpp"
#include <unistd.h>
#include <iostream>

struct intpair {
	intptr_t first, second;
	intpair() = default;
	intpair(int f, int s) {
		first = f;
		second = s;
	}
};

intpair operator+(const intpair& a, const intpair& b) {
	return intpair(a.first + b.first, a.second + b.second);
}

// Snapshot should be a snapshot on intpair
struct reducing_snapshot_bench : public benchmark {
	typedef reducing_snapshot<intptr_t> Snapshot;

	int thread_count;

	std::unique_ptr<Snapshot> summer;

	void thread(int thread_idx) {
		intptr_t i = 1;
		while (should_continue()) {
			summer->update(thread_idx, &i);
			i++;
			counter::inc_counter(counter::Parsum_Ops, 1);
		}
	}

	void execute(int seconds) {
		std::cout << "Thread count: " << thread_count << std::endl;
		std::cout << "Benchmark duration [s]: " << seconds << std::endl;
		run(seconds, thread_count);
		int op_count = counter::get_counter(counter::Parsum_Ops);
		double duration = (1000.0 * 1000.0 * seconds * thread_count) / op_count;
		std::cout << "Operation duration [us]: " << duration << std::endl;
		std::cout << "Updates with own publish: " << ((double)counter::get_counter(counter::Parsum_OwnPublish)) / counter::get_counter(counter::Parsum_Update) << std::endl;
		std::cout << "Full update_lasts: " << ((double)counter::get_counter(counter::Parsum_UpdateLastExec)) / counter::get_counter(counter::Parsum_UpdateLastCall) << std::endl;
		std::cout << "Full update_lasts per update: " << ((double)counter::get_counter(counter::Parsum_UpdateLastExec)) / counter::get_counter(counter::Parsum_Update) << std::endl;
		std::cout << "Fraction of successful history updates: " << ((double)counter::get_counter(counter::History_PublishSucc)) / counter::get_counter(counter::History_PublishCall) << std::endl;
		std::cout << std::endl;
	}

	reducing_snapshot_bench(int t) :
		thread_count(t),
		summer(Snapshot::create(t))
	{ }
};

int main(int argc, char **argv)
{
	for(int i=1;i<argc;i++) {
		int t = atoi(argv[i]);
		if (t == 0) continue;
		reducing_snapshot_bench b(t);
		b.execute(20);
	}
}
		
