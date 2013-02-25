#ifndef __UTIL_GUARD
#define __UTIL_GUARD

#include <memory>
#include <algorithm>
#include <cstring>
#include <cstddef>
#include <sstream>
#include "common.hpp"

template<typename T> class permuted_array {
	private:
		static const int CACHELINE_SIZE = 128;
		int size_;
		int step_;
		std::unique_ptr<T[]> ptr_;
	public:
		permuted_array(int size = 0) :
			/* We want size to be odd so that step of 1<<something will be OK */
			size_(size|1),
			ptr_(new T[size_]) {
				step_ = 1;
				while (step_*sizeof(T) < CACHELINE_SIZE) step_ <<=1;
				// We might end up with step_ >= size -- then we will not be able to fit stuff nicely, but step_ will still
				// be relatively prime with size_ and we don't care whether it's larger then size_.
			}
		T& operator[](int idx)
		{
			return ptr_[(idx*step_)%size_];
		}

		const T& operator[](int idx) const
		{
			return ptr_[(idx*step_)%size_];
		}
};

class log_stream : public std::stringstream {
	public:
		log_stream(rl::debug_info_param info) : info_(info) { }
		~log_stream() {
			rl::ctx().exec_log_msg(info_, str().c_str());
		}
	private:
		rl::debug_info_param info_;
};

#define LOG (log_stream(RL_INFO))

#endif

