#ifndef __LARGE_ATOMIC_GUARD
#define __LARGE_ATOMIC_GUARD

#include "common.hpp"
#include <utility>
#include <iostream>

#ifdef RELACY
template<typename F, typename S> class large_atomic {
	private:
		static_assert(sizeof(F) <= sizeof(word), "large_atomic first element too large");
		static_assert(sizeof(S) <= sizeof(word), "large_atomic second element too large");
		std::atomic<std::pair<F, S> > contents_;
	public:
		large_atomic(const std::pair<F, S> init_value = std::pair<F, S>()) : contents_(init_value) {}
		template<typename... Ts> F load_1(Ts... as...) {
			return contents_.load(as...).first;
		}
		template<typename... Ts> F load_2(Ts... as...) {
			return contents_.load(as...).second;
		}
		template<typename... Ts> void store_1(const F& val, Ts... as...) { // fixme -- is memory order handling correct?
			std::pair<F, S> p = contents_.load(as...);
			while (!contents_.compare_exchange_strong(p, std::make_pair(val, p.second), as...))
				;
		}
		template<typename... Ts> void store_2(const F& val, Ts... as...) { // fixme -- is memory order handling correct?
			std::pair<F, S> p = contents_.load(as...);
			while (!contents_.compare_exchange_strong(p, std::make_pair(p.first, val), as...))
				;
		}
		template<typename... Ts> bool compare_exchange_strong(Ts... as...) {
			return contents_.compare_exchange_strong(as...);
		}
};

namespace std {
template<typename F, typename S> std::ostream& operator<<(std::ostream& str, std::pair<F, S> p)
{
	return (str << "(" << p.first << "," << p.second << ")");
}
}
#else // RELACY
// In the following, we rely on the way std::atomic is implemented in GCC on x86_64.

template<typename F, typename S> class large_atomic {
	private:
		dword storage;
		static_assert(sizeof(std::atomic<dword>) == sizeof(dword), "Atomic implementation doesn't match assumptions");
		static_assert(sizeof(std::atomic<word>) == sizeof(word), "Atomic implementation doesn't match assumptions");
		std::atomic<dword> *all() { return reinterpret_cast<std::atomic<dword>* >(&storage); };
		std::atomic<word> *first() { return reinterpret_cast<std::atomic<word>* >(&storage); };
		std::atomic<word> *second() { return (reinterpret_cast<std::atomic<word>* >(&storage) + 1); };

		dword pack(std::pair<F, S> v) {
			dword r;
			reinterpret_cast<word*>(&r)[0] = v.first;
			reinterpret_cast<word*>(&r)[1] = v.second;
			return r;
		}

		std::pair<F, S> unpack(dword v) {
			std::pair<F, S> p;
			p.first = reinterpret_cast<word*>(&v)[0];
			p.second = reinterpret_cast<word*>(&v)[1];
			return p;
		}

	public:
		large_atomic(std::pair<F, S> init_value = std::pair<F, S>()) {
			new (&storage) std::atomic<dword>;
			new (reinterpret_cast<word*>(&storage)) std::atomic<word>;
			new (reinterpret_cast<word*>(&storage)+1) std::atomic<word>;
			all()->store(pack(init_value), std::memory_order_seq_cst);
		}

		F load_1(std::memory_order mo) {
			return first()->load(mo);
		}

		F load_2(std::memory_order mo) {
			return second()->load(mo);
		}

		void store_1(const F& v, std::memory_order mo) {
			first()->store(v, mo);
		}

		void store_2(const F& v, std::memory_order mo) {
			second()->store(v, mo);
		}

		bool compare_exchange_strong(std::pair<F, S>& expected, std::pair<F, S> desired, std::memory_order mo) {
			dword exp_raw = pack(expected);
			bool ret = all()->compare_exchange_strong(exp_raw, pack(desired), mo);
			expected = unpack(exp_raw);
			return ret;
		}
};
#endif // !RELACY

#endif
