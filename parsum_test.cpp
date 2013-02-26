#include "parsum.hpp"

const int thread_count = 5;
const int op_count = 2;

struct reducing_snapshot_test : public rl::test_suite<reducing_snapshot_test, thread_count> {
	reducing_snapshot<intptr_t> summer;
	VAR_T(bool) done[thread_count * op_count];

	void thread(int thread_idx) {
		for(int i=1;i<=op_count;i++) {
			intptr_t new_value = i;
			reducing_snapshot<intptr_t>::update_result res = summer.update(thread_idx, &new_value);
			VAR(done[res.value_before]) = true;
		}
	}

	reducing_snapshot_test() : summer(thread_count) { }
};


int main()
{
	rl::test_params p;
	p.execution_depth_limit = 5000;
	rl::simulate<reducing_snapshot_test>(p);
}
		
