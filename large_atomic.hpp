#include <atomic>
#include <new>


class large_atomic {
	private:
		__uint128_t storage;
		std::atomic<__uint128_t> &all() { return *reinterpret_cast<std::atomic<__uint128_t>* >(&storage); };
		std::atomic<unsigned long long> &first_half() { return *reinterpret_cast<std::atomic<unsigned long long>* >(&storage); };
		std::atomic<unsigned long long> &second_half() { return *(reinterpret_cast<std::atomic<unsigned long long>* >(&storage) + 1); };
		template<int N> friend std::atomic<unsigned long long>& _large_atomic_half(large_atomic&);
		template<int N> std::atomic<unsigned long long>& half() { return _large_atomic_half<N>(*this); }
	public:
		large_atomic() {
			new (&storage) std::atomic<__uint128_t>;
			new (reinterpret_cast<unsigned long long*>(&storage)) std::atomic<unsigned long long>;
			new (reinterpret_cast<unsigned long long*>(&storage)+1) std::atomic<unsigned long long>;
		}

		template<int N> unsigned long long load(std::memory_order mo) {
			return half<N>().load(mo);
		}

		template<int N> void store(unsigned long long v, std::memory_order mo) {
			half<N>().store(v, mo);
		}

		bool compare_exchange_strong(__uint128_t exp, __uint128_t des, std::memory_order mo) {
			return all().compare_exchange_strong(exp, des, mo);
		}

		__uint128_t compare_exchange_strong_value(__uint128_t exp, __uint128_t des, std::memory_order mo) {
			all().compare_exchange_strong(exp, des, mo);
			return exp;
		}
};

template<int N> std::atomic<unsigned long long>& _large_atomic_half(large_atomic& a) {
	// TODO: break this
}
template<> std::atomic<unsigned long long>& _large_atomic_half<0>(large_atomic& a) {
	return a.first_half();
}
template<> std::atomic<unsigned long long>& _large_atomic_half<1>(large_atomic& a) {
	return a.second_half();
}




