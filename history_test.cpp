#include "history.hpp"

struct history_test {
	struct element {
		intptr_t ver;
		intptr_t tid;
		intptr_t sum;

		element(intptr_t v, intptr_t t) : ver(v), tid(t), sum(v+t) { }
		element() = default;
	};

	bool valid(element e) {
		return e.ver + e.tid == e.sum && 0 <= e.tid && e.tid < thread_count;
	}

	int thread_count;
	int history_size;
	int reps;
	
	history<element, raw_locked_history> hist;
	std::unique_ptr<VAR_T(bool)[]> done;

	typedef raw_history::version_t version_t;

	history_test(int t, int h, int r) : thread_count(t), hist(h, t), history_size(h), reps(r), done(new VAR_T(bool)[r*h]) { }

	void check_single_ver(version_t ver) {
		element value;
		bool ok = hist.get(ver, &value);
		if (ok) {
			assert(valid(value));
			assert(value.ver == ver);
		} else {
			version_t current_ver = hist.get_version();
//			LOG << "current ver " << current_ver;
			assert(current_ver - ver >= history_size);
		}
	}

	void before() {
		for(int i=1;i<reps*history_size;i++)
			VAR(done[i]) = false;
	}

	void thread(int thread_idx) {
		for(version_t i=hist.get_version()+1; i<reps*history_size; i++) {
			element value(i, thread_idx);
			bool ok = hist.publish(thread_idx, i, &value);
			if (ok) {
				assert(!VAR(done[i])); // We need not worry about races on done[i] -- they cause the test to fail.
				VAR(done[i]) = true;
			}
//			LOG << "checking " << i;
			check_single_ver(i);
		}
	}

	void after() {
		for(int i=1;i<reps*history_size;i++)
			assert(VAR(done[i]));
	}
};

RELACY_FIXTURE(history_fixture, history_test, 5, 5, 2);

int main()
{
#ifdef RELACY
	rl::simulate<history_fixture>();
#else
	run_test<history_test>(2, 500, 20000);
#endif
}

