#include "holder.hpp"

const int thread_count = 3;

struct holder_test : public rl::test_suite<holder_test, thread_count> {
	struct element {
		intptr_t ver1;
		intptr_t ver2;

		element(intptr_t v) : ver1(v), ver2(v) {}
		element() = default;
		bool ok() {
			return ver1 == ver2;
		}
	};
	
	holder<element> hold;

	void thread(int thread_idx) {
		intptr_t prev_ver = 0;
		while(prev_ver < 10) {
			element value;
			intptr_t ver = hold.get(&value);
			if (ver == holder<element>::INVALID_VERSION)
				continue;
			RL_ASSERT(ver >= prev_ver);
			RL_ASSERT(value.ok());
			RL_ASSERT(value.ver1 == ver);
			value = element(ver + 1);
			hold.update(ver + 1, &value);
			prev_ver = ver + 1;
		}
	}
};

int main()
{
	rl::simulate<holder_test>();
}

