#include "history.hpp"

const int thread_count = 3;
const int history_size = 10;

struct history_test : public rl::test_suite<history_test, thread_count> {
	history<intptr_t> hist;
	VAR_T(bool) done[2*history_size];

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

	void before() {
		for(int i=1;i<2*history_size;i++)
			VAR(done[i]) = false;
	}

	void thread(int thread_idx) {
		for(version_t i=hist.get_version()+1; i<2*history_size; i++) {
			intptr_t value = i;
			bool ok = hist.publish(thread_idx, i, &value);
			if (ok) {
				RL_ASSERT(!VAR(done[i])); // We need not worry about races on done[i] -- they cause the test to fail.
				VAR(done[i]) = true;
			}
			LOG << "checking " << i;
			check_single_ver(i);
		}
	}

	void after() {
		for(int i=1;i<2*history_size;i++)
			RL_ASSERT(VAR(done[i]));
	}
};

int main()
{
	rl::simulate<history_test>();
}

