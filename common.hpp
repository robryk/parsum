#ifndef __COMMON_GUARD
#define __COMMON_GUARD

#define unlikely(x) (x)
typedef unsigned long long word;
typedef __uint128_t dword;

#ifdef RELACY
#include "relacy/relacy_std.hpp"
// assert(x) is defined in relacy
// VAR_T is defined in relacy
// VAR is defined in relacy
// $ is defined in relacy
#define USING_MEMORY_ORDERS using rl::mo_seq_cst; using rl::mo_acquire; using rl::mo_release; using rl::mo_relaxed

#define RELACY_FIXTURE(name, thread_count, ...) \
struct name##_fixture : public rl::test_suite<name##_fixture, thread_count>, public name { \
	void before() { name::before(); } \
	void after() { name::after(); } \
	void invariant() { /*history_test::invariant();*/ } \
	void thread(int thread_idx) { name::thread(thread_idx); } \
	name##_fixture() : name(thread_count, ##__VA_ARGS__) { } \
}

#else

#include <cassert>
#include <atomic>
#include <mutex>
#include <thread>
#define VAR_T(t) t
#define VAR(x) x
#define $
#define USING_MEMORY_ORDERS using std::memory_order_seq_cst; using std::memory_order_acquire; using std::memory_order_release; using std::memory_order_relaxed
template<typename T, typename... Args> void run_test(int thread_count, Args... args) {
	T obj(thread_count, std::forward<Args>(args)...);
	obj.before();
	std::thread thrs[thread_count];
	for(int i=0;i<thread_count;i++)
		thrs[i] = std::thread(&T::thread, &obj, i);
	for(int i=0;i<thread_count;i++)
		thrs[i].join();
	obj.after();
}

#define RELACY_FIXTURE(...)
	
#endif

#endif

