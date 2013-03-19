#include "parsum.hpp"
#include "counter.hpp"
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
struct reducing_snapshot_test {
	typedef reducing_snapshot<intptr_t> Snapshot;

	int thread_count;

	std::unique_ptr<Snapshot> summer;
	std::atomic<bool> start_;
	std::atomic<bool> stop_;
	intptr_t op_count_;
	std::mutex op_count_mu_;

	void thread(int thread_idx) {
		intptr_t i = 1;
		while (!start_.load(std::memory_order_relaxed)) {
		}
		while (!stop_.load(std::memory_order_relaxed)) {
			summer->update(thread_idx, &i);
			i++;
			counter::inc_counter(counter::Parsum_Ops, 1);
		}
		//op_count_mu_.lock();
		//op_count_ += i - 1;
		//op_count_mu_.unlock();
		counter::finalize();
	}

	void execute(int seconds) {
		std::thread threads[thread_count];
		counter::reset();
		start_.store(false);
		stop_.store(false);
		op_count_ = 0;
		for(int i=0;i<thread_count;i++)
			threads[i] = std::thread(&reducing_snapshot_test::thread, this, i);
		sleep(1);
		start_.store(true);
		sleep(seconds);
		stop_.store(true);
		for(int i=0;i<thread_count;i++)
			threads[i].join();
		int op_count = counter::get_counter(counter::Parsum_Ops);
		double duration = (1000.0 * 1000.0 * seconds * thread_count) / op_count;
		std::cout << duration << "us/op" << std::endl;
		std::cout << "Updates with own publish: " << ((double)counter::get_counter(counter::Parsum_OwnPublish)) / counter::get_counter(counter::Parsum_Update) << std::endl;
		std::cout << "Full update_lasts: " << ((double)counter::get_counter(counter::Parsum_UpdateLastExec)) / counter::get_counter(counter::Parsum_UpdateLastCall) << std::endl;
		std::cout << "Full update_lasts per update: " << ((double)counter::get_counter(counter::Parsum_UpdateLastExec)) / counter::get_counter(counter::Parsum_Update) << std::endl;
		std::cout << "Fraction of successful history updates: " << ((double)counter::get_counter(counter::History_PublishSucc)) / counter::get_counter(counter::History_PublishCall) << std::endl;
	}

	reducing_snapshot_test(int t) :
		thread_count(t),
		summer(Snapshot::create(t))
	{ }
};

int main()
{
	int nt;
	std::cin >> nt;
	reducing_snapshot_test t(nt);
	t.execute(20);
}
		
