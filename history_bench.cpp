#include <unistd.h>
#include "history.hpp"
#include "counter.hpp"
#include "bench.hpp"

template<int N> struct history_bench : public benchmark {
	struct element {
		intptr_t v[N];
	};

	int thread_count;
	int history_size;
	
	history<element, raw_history> hist;

	std::atomic<bool> start_;
	std::atomic<bool> stop_;

	typedef raw_history::version_t version_t;

	history_bench(int t, int h) : thread_count(t), hist(h, t), history_size(h) { }

	void thread(int thread_idx) {
		element e;
		while (should_continue()) {
			version_t v = hist.get_version();
			hist.publish(thread_idx, v+1, &e);
		}
	}

	void execute(int seconds) {
		std::cout << "History size: " << N << std::endl;
		std::cout << "Thread count: " << thread_count << std::endl;
		std::cout << "Benchmark duration [s]: " << seconds << std::endl;
		run(seconds, thread_count);
		int op_count = counter::get_counter(counter::History_PublishCall);
		double duration = (1000.0 * 1000.0 * seconds * thread_count) / op_count;
		std::cout << "Operation duration [us]: " << duration << std::endl;
		std::cout << "Fraction of successful history updates: " << ((double)counter::get_counter(counter::History_PublishSucc)) / counter::get_counter(counter::History_PublishCall) << std::endl;
		std::cout << std::endl;
	}

};

int main(int argc, char **argv)
{
	for(int i=1;i<argc;i++) {
		int t = atoi(argv[i]);
		if (t == 0)
			continue;
		history_bench<1> b1(t, 20);
		b1.execute(20);
		history_bench<2> b2(t, 20);
		b2.execute(20);
		history_bench<5> b5(t, 20);
		b5.execute(20);
	}
}

