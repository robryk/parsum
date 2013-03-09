#include "holder.hpp"

//const int thread_count = 3;

struct holder_test {
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

	void before() {
	}

	void thread(int thread_idx) {
		intptr_t prev_ver = 0;
		while(prev_ver < 10) {
			element value;
			intptr_t ver = hold.get(&value);
			if (ver == holder<element>::INVALID_VERSION)
				continue;
			assert(ver >= prev_ver);
			assert(value.ok());
			assert(value.ver1 == ver);
			value = element(ver + 1);
			hold.update(ver + 1, &value);
			prev_ver = ver + 1;
		}
	}

	void after() {
	}

	holder_test(int thread_count) {}
};

RELACY_FIXTURE(holder_fixture, holder_test, 3);

int main()
{
#ifdef RELACY
	rl::simulate<holder_fixture>();
#else
	run_test<holder_test>(3);
#endif
}

