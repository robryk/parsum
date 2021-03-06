#include "parsum.hpp"

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
template<typename Snapshot> struct reducing_snapshot_test {

	int thread_count;
	int op_count;

	std::unique_ptr<Snapshot> summer;
	std::unique_ptr<VAR_T(bool)[]> done1;
	std::unique_ptr<VAR_T(bool)[]> done2;

	void before() {
		for(int i=0;i<thread_count * op_count;i++)
			VAR(done1[i]) = false;
		for(int i=0;i<thread_count * op_count;i++)
			VAR(done2[i]) = false;
	}

	void after() {
		for(int i=0;i<thread_count * op_count;i++)
			assert(VAR(done1[i]));
		for(int i=0;i<thread_count * op_count;i++)
			assert(VAR(done2[i]));
	}

	void thread(int thread_idx) {
		for(int i=1;i<=op_count;i++) {
			intpair new_value(i, 0);
			typename Snapshot::update_result res = summer->update(thread_idx, &new_value);
			assert(VAR(done1[res.value_before.first]) == false);
			VAR(done1[res.value_before.first]) = true;
		}
		for(int i=1;i<=op_count;i++) {
			intpair new_value(op_count, i);
			typename Snapshot::update_result res = summer->update(thread_idx, &new_value);
			assert(VAR(done2[res.value_before.second]) == false);
			VAR(done2[res.value_before.second]) = true;
		}
	}

	reducing_snapshot_test(int t, int o) :
		thread_count(t),
		op_count(o),
		summer(Snapshot::create(t)),
		done1(new VAR_T(bool)[t*o]),
		done2(new VAR_T(bool)[t*o])
	{ }
};

typedef reducing_snapshot_test<reducing_snapshot<intpair, raw_locked_history, raw_locked_holder> > test_locked;
RELACY_FIXTURE(fixture_locked, test_locked, 5, 2);
RELACY_FIXTURE(fixture_lockfree, reducing_snapshot_test<reducing_snapshot<intpair> >, 3, 2);

int main()
{
#ifdef RELACY
	rl::test_params p;
	p.execution_depth_limit = 5000;
	rl::simulate<fixture_locked>(p);
	rl::simulate<fixture_lockfree>(p);
#else
	run_test<reducing_snapshot_test<reducing_snapshot<intpair> > >(5, 20000000);
#endif
}
		
