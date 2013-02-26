#include "parsum.hpp"

const int thread_count = 3;
const int op_count = 2;

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

struct reducing_snapshot_test : public rl::test_suite<reducing_snapshot_test, thread_count> {

	std::unique_ptr<reducing_snapshot<intpair> > summer;
	VAR_T(bool) done1[thread_count * op_count];
	VAR_T(bool) done2[thread_count * op_count];

	void before() {
		for(int i=0;i<sizeof(done1)/sizeof(VAR_T(bool));i++)
			VAR(done1[i]) = false;
		for(int i=0;i<sizeof(done2)/sizeof(VAR_T(bool));i++)
			VAR(done2[i]) = false;
	}

	void after() {
		for(int i=0;i<sizeof(done1)/sizeof(VAR_T(bool));i++)
			RL_ASSERT(VAR(done1[i]));
		for(int i=0;i<sizeof(done2)/sizeof(VAR_T(bool));i++)
			RL_ASSERT(VAR(done2[i]));
	}


	void thread(int thread_idx) {
		for(int i=1;i<=op_count;i++) {
			intpair new_value(i, 0);
			reducing_snapshot<intpair>::update_result res = summer->update(thread_idx, &new_value);
			RL_ASSERT(VAR(done1[res.value_before.first]) == false);
			VAR(done1[res.value_before.first]) = true;
		}
		for(int i=1;i<=op_count;i++) {
			intpair new_value(op_count, i);
			reducing_snapshot<intpair>::update_result res = summer->update(thread_idx, &new_value);
			RL_ASSERT(VAR(done2[res.value_before.second]) == false);
			VAR(done2[res.value_before.second]) = true;
		}
	}

	reducing_snapshot_test() : summer(reducing_snapshot<intpair>::create(thread_count)) { }
};




int main()
{
	rl::test_params p;
	p.execution_depth_limit = 5000;
	rl::simulate<reducing_snapshot_test>(p);
}
		
