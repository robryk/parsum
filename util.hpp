#ifndef __UTIL_GUARD
#define __UTIL_GUARD

#include <memory>
#include <algorithm>
#include <cstring>
#include <cstddef>
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

template<typename T> class word_access {
	private:
		typedef typename std::remove_reference<T>::type type;
		static constexpr int is_const = std::is_const<type>::value;
		class reference {
			private:
				void* pos_;
				int count_;
				reference(void* pos, int count) : pos_(pos), count_(count) {}
				friend class word_access;
			public:
				void operator=(word v) {
					memcpy(pos_, &v, count_);
				}

				operator word() const {
					word ret = 0;
					memcpy(&ret, pos_, count_);
					return ret;
				}
		};
		T value_;
	public:
		static const int Size = (sizeof(T)+sizeof(word)-1)/sizeof(word);
		word_access(T value) : value_(value) { }
		typename std::conditional<is_const, const reference, reference>::type operator[](int idx) {
			// If type is const something, we return const reference which disallows write attempts. const_cast below is to create
			// a reference of the same type at all (references would need to have completely distinct types otherwise)
			return reference(const_cast<word*>(reinterpret_cast<const word*>(&value_) + idx), std::min(sizeof(word), sizeof(type) - idx*sizeof(word)));
		}
};

template<typename T, typename Source> class word_proxy {
	private:
		Source source_;
	public:
		word_proxy(Source source) : source_(source) {}
		template<typename U> U operator->*(U T::* ptr) {
			word buf[sizeof(U)/sizeof(word) + 3*sizeof(word)];
			ptrdiff_t offset = reinterpret_cast<ptrdiff_t>(&(reinterpret_cast<T const volatile*>(NULL)->*ptr));
			for(int i=offset/sizeof(word);i*sizeof(word)<offset + sizeof(U);i++)
				buf[i - offset/sizeof(word)] = source_[i];
			return *(reinterpret_cast<U*>(reinterpret_cast<char*>(buf) + (offset % sizeof(word))));
		}
		T operator*() {
			word buf[(sizeof(T)+sizeof(word)-1)/sizeof(word)];
			for(int i=0;i<(sizeof(T)+sizeof(word)-1)/sizeof(word);i++)
				buf[i] = source_[i];
			return *(reinterpret_cast<T*>(buf));
		}
};

#endif

