#include "history.hpp"

const int thread_count = 3;
const int history_size = 10;

struct history_test : public rl::test_suite<history_test, thread_count> {
	history<intptr_t> hist;

	typedef raw_history::version_t version_t;

	history_test() : hist(history_size, thread_count) { }

	void check_single_ver(version_t ver) {
		intptr_t value;
		bool ok = hist.get(ver, &value);
		if (ok) {
			RL_ASSERT(value == ver);
		} else {
			version_t current_ver = hist.get_version();
			LOG << "current ver " << current_ver;
			RL_ASSERT(current_ver - ver >= history_size);
		}
	}

	void thread(int thread_idx) {
		for(version_t i=hist.get_version()+1; i<2*history_size; i++) {
			intptr_t value = i;
			hist.publish(thread_idx, i, &value);
			LOG << "checking " << i;
			check_single_ver(i);
		}
	}
};

int main()
{
	rl::simulate<history_test>();
}

