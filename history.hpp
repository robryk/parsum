#include <cstring>
#include <algorithm>
#include <memory>

#include "util.hpp"
#include "large_atomic.hpp"

#define ATOMIC(x) std::atomic< x >


class raw_history {
	public:
		typedef intptr_t version_t;
		typedef intptr_t data_t;
		typedef intptr_t tid_t;

	private:
		const int element_size_;
		const int history_size_;
		const int thread_count_;

		struct latest_t {
			std::unique_ptr<ATOMIC(uintptr_t)[]> contents;
			ATOMIC(uintptr_t)& version() {
				return contents[0];
			}
			ATOMIC(uintptr_t)& data(int i) {
				return contents[i+1];
			}
		};

		large_atomic<version_t, tid_t> current_version_;
		
		std::unique_ptr<std::unique_ptr<large_atomic<version_t, data_t>[]>[]> buffer_;
		std::unique_ptr<latest_t[]> latest_;

		bool get_from_latest(version_t ver, tid_t creator, data_t* output);
		bool get_from_buffer(version_t ver, data_t* output);

	public:
		version_t get_version() {
			using rl::mo_seq_cst;
			return current_version_.load_1(memory_order_seq_cst);
		}

		bool get(version_t ver, data_t* output);
		bool publish(tid_t me, version_t ver, const data_t* input);
		raw_history(int element_size, int history_size, int thread_count);
};

template<typename T, typename Raw = raw_history> class history {
	private:
		Raw hist;

		typedef typename Raw::data_t data_t;
		static constexpr size_t element_size = (sizeof(T) + sizeof(data_t) - 1) / sizeof(data_t);
	public:
		
		typedef typename Raw::version_t version_t;
		typedef typename Raw::tid_t tid_t;

		history(int history_size, int thread_count) : hist(element_size, history_size, thread_count) { }

		version_t get_version() {
			return hist.get_version();
		}

		bool get(version_t ver, T* output) {
			intptr_t buffer[element_size];
			bool ok = hist.get(ver, buffer);
			if (ok)
				memcpy(output, buffer, sizeof(T));
			return ok;
		}

		bool publish(tid_t me, version_t ver, const T* input) {
			intptr_t buffer[element_size];
			memcpy(buffer, input, sizeof(T));
			return hist.publish(me, ver, buffer);
		}
};

