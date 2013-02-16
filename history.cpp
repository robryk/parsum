#include "history.hpp"

const int thread_count = 3;
const int history_size = 10;

struct history_test : public rl::test_suite<history_test, thread_count> {
	struct val {
		char c;
		val(char _c) : c(_c) {}
		val() = default;
	};
	history<val> hist;

	history_test() : hist(history_size, thread_count, 0) { }

	void check_single_ver(int ver) {
		try {
			char c = hist.get(ver)->*(&val::c);
			RL_ASSERT(c == (char)ver);
		} catch (history<val>::too_old_exception) {
			int current_ver = hist.get_ver();
			RL_ASSERT(current_ver - ver >= history_size);
		}
	}

	void thread(int thread_idx) {
		for(int i=hist.get_ver(); i<2*history_size; i++) {
			val v; v.c = i+1;
			hist.publish(i, thread_idx, v);
			check_single_ver(i+1);
		}
	}
};

int main()
{
	rl::simulate<history_test>();
}

