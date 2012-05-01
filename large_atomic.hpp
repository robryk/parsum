#include <atomic>
#include <utility>
#include "common.hpp"

template<typename F, typename S> class large_atomic {
	private:
		static_assert(sizeof(F) <= sizeof(word), "large_atomic first element too large");
		static_assert(sizeof(S) <= sizeof(word), "large_atomic second element too large");
		std::atomic<std::pair<F, S> > contents_;
	public:
		large_atomic(const std::pair<F, S> init_value) : contents_(init_value) {}
		large_atomic() = default;
		template<typename... Ts> F load_1(Ts... as...) {
			return contents_.load(as...).first;
		}
		template<typename... Ts> F load_2(Ts... as...) {
			return contents_.load(as...).second;
		}
		template<typename... Ts> void store_1(const F& val, Ts... as...) { // fixme -- is memory order handling correct?
			std::pair<F, S> p = contents_.load(as...);
			while (!contents_.compare_exchange(p, std::make_pair(val, p.second), as...))
				;
		}
		template<typename... Ts> void store_2(const F& val, Ts... as...) { // fixme -- is memory order handling correct?
			std::pair<F, S> p = contents_.load(as...);
			while (!contents_.compare_exchange(p, std::make_pair(p.first, val), as...))
				;
		}
		template<typename... Ts> bool compare_exchange_strong(Ts... as...) {
			return contents_.compare_exchange_strong(as...);
		}
};

/*class large_atomic {
	private:
		large_int storage;
		std::atomic<large_int> &all() { return *reinterpret_cast<std::atomic<large_int>* >(&storage); };
		std::atomic<big_int> &first_half() { return *reinterpret_cast<std::atomic<big_int>* >(&storage); };
		std::atomic<big_int> &second_half() { return *(reinterpret_cast<std::atomic<big_int>* >(&storage) + 1); };
		template<int N> friend std::atomic<big_int>& _large_atomic_half(large_atomic&);
		template<int N> std::atomic<big_int>& half() { return _large_atomic_half<N>(*this); }
	public:
		struct value {
			
		large_atomic() {
			new (&storage) std::atomic<large_int>;
			new (reinterpret_cast<big_int*>(&storage)) std::atomic<big_int>;
			new (reinterpret_cast<big_int*>(&storage)+1) std::atomic<big_int>;
		}

		template<int N> big_int load(std::memory_order mo) {
			return half<N>().load(mo);
		}

		template<int N> void store(big_int v, std::memory_order mo) {
			half<N>().store(v, mo);
		}

		bool compare_exchange_strong(large_int exp, large_int des, std::memory_order mo) {
			return all().compare_exchange_strong(exp, des, mo);
		}

		large_int compare_exchange_strong_value(large_int exp, large_int des, std::memory_order mo) {
			all().compare_exchange_strong(exp, des, mo);
			return exp;
		}
};

template<int N> std::atomic<big_int>& _large_atomic_half(large_atomic& a) {
	// TODO: break this
}
template<> std::atomic<big_int>& _large_atomic_half<0>(large_atomic& a) {
	return a.first_half();
}
template<> std::atomic<big_int>& _large_atomic_half<1>(large_atomic& a) {
	return a.second_half();
}*/


