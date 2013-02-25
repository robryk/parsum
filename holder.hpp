#include "util.hpp"
#include "large_atomic.hpp"

class raw_holder {
	private:
		const int element_size_;
		std::unique_ptr<large_atomic<intptr_t, intptr_t>[]> contents_;
	public:
		raw_holder(int element_size);

		static constexpr int INVALID_VERSION = -1;

		intptr_t get(intptr_t* value);
		// Precondition: For every lower version ver': If an update(ver') has started, at least one update(ver') has finished
		// Precondition: All calls to update(ver) for a given ver have equal values
		void update(intptr_t version, const intptr_t* value);
};

class raw_locked_holder {
	private:
		const int element_size_;

		std::mutex m_;
		VAR_T(intptr_t) version_;
		std::unique_ptr<VAR_T(intptr_t)[]> contents_;
	public:
		raw_locked_holder(int element_size);

		static constexpr int INVALID_VERSION = -1;

		intptr_t get(intptr_t* value);
		void update(intptr_t version, const intptr_t* value);
};
		

template<typename T, typename Raw = raw_holder> class holder {
	private:
		Raw holder_;
		static constexpr int element_size_ = (sizeof(T) + sizeof(intptr_t) - 1) / sizeof(intptr_t);
	public:
		holder() : holder_(element_size_) { }

		static constexpr int INVALID_VERSION = Raw::INVALID_VERSION;

		intptr_t get(T* value) {
			intptr_t buffer[element_size_];
			intptr_t result = holder_.get(buffer);
			memcpy(value, buffer, sizeof(T));
			return result;
		}

		void update(intptr_t version, const T* value) {
			intptr_t buffer[element_size_];
			memcpy(buffer, value, sizeof(T));
			holder_.update(version, buffer);
		}
};


