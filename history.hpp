#include <cstring>
#include <algorithm>
#include <atomic>
#include <memory>
#include "large_atomic.hpp"

//typedef __uint128_t large_int;
//typedef unsigned long long big_int;

typedef unsigned long long large_int;
typedef unsigned int big_int;

const int field_size = sizeof(big_int);

template<typename Struct> class word_access {
	private:
		Struct& value_;
		class reference {
			private:
				void* pos_;
				int count_;
				reference(void* pos, int count) : pos_(pos), count_(count) {}
				friend class word_access;
			public:
				void operator=(big_int v) {
					memcpy(pos_, &v, count_);
				}

				operator big_int() const {
					big_int ret = 0;
					memcpy(&ret, pos_, count_);
					return ret;
				}
		};

	public:
		static const int STRUCT_SIZE = (sizeof(Struct) + field_size - 1) / field_size;
		word_access(const Struct& value) : value_(value) {}
		big_int operator[](int idx) const {
			big_int ret = 0;
			memcpy(&ret, reinterpret_cast<const big_int*>(&value_) + idx, std::min(sizeof(big_int), sizeof(Struct) - idx*sizeof(big_int)));
			return ret;
		}

		reference operator[](int idx) {
			// fixme -- prevent when Struct is const
			return reference(const_cast<big_int*>(reinterpret_cast<const big_int*>(&value_)) + idx, std::min(sizeof(big_int), sizeof(Struct) - idx*sizeof(big_int)));
		}

};
			

template<typename Struct> class history {
	private:
		static const int STRUCT_SIZE = word_access<Struct>::STRUCT_SIZE;

		struct history_record {
			large_atomic value[STRUCT_SIZE];
			// pad to cacheline would be handy
		};

		struct version_tid {
			big_int version;
			big_int tid;
			version_tid(big_int _version, big_int _tid) : version(_version), tid(_tid) {}
			version_tid(large_int raw = 0) : version(static_cast<big_int>(raw >> sizeof(big_int))), tid(static_cast<big_int>(raw)) {}
			operator large_int() const { return (static_cast<large_int>(version) << sizeof(big_int)) | static_cast<large_int>(tid); }
			large_int raw() const { return static_cast<large_int>(*this); }
		};

		struct history_element {
			big_int version;
			big_int value;
			history_element(big_int _version, big_int _value) : version(_version), value(_value) {}
			history_element(large_int raw = 0) : version(static_cast<big_int>(raw >> sizeof(big_int))), value(static_cast<big_int>(raw)) {}
			operator large_int() const { return (static_cast<large_int>(version) << sizeof(big_int)) | static_cast<large_int>(value); }
			large_int raw() const { return static_cast<large_int>(*this); }
		};

		const int H_;
		large_atomic current_version_;
		std::unique_ptr<history_record[]> history_;
		std::unique_ptr<history_record[]> vals_;

		void help(version_tid current_version) {
			for(int i=0;i<STRUCT_SIZE;i++) {
				large_atomic& val = vals_[current_version.tid].value[i];
				/* This check might be unnecessary. If we first find that a version is live, then find its number in publisher's vals[]
				 * we can be sure that inbetween publisher's vals contained correct version
				 */
				//if (val.load<0>(memory_order_seq_cst) != current_version.version)
				//	return;
				big_int value = val.load<1>(memory_order_seq_cst);
				if (val.load<0>(memory_order_seq_cst) != current_version.version)
					return;
				large_atomic& element = history_[current_version.version % H_].value[i];
				history_element old_element;
				old_element.version = element.load<0>(memory_order_seq_cst);
				old_element.value = element.load<1>(memory_order_seq_cst);
				if (old_element.version > current_version.version)
					return;
				if (old_element.version < current_version.version)
					element.compare_exchange_strong(old_element.raw(), history_element(current_version.version, value).raw(), memory_order_seq_cst);
			}
		}
		
	public:
		history(int H, int T, const Struct& initial_value) : H_(H), history_(new history_record[H]), vals_(new history_record[T]) {
			current_version_.store<0>(0, memory_order_seq_cst);
			current_version_.store<1>(0, memory_order_seq_cst);
			for(int i=0;i<H;i++)
				for(int j=0;j<STRUCT_SIZE;j++)
					history_[i].value[j].store<0>(i+1, memory_order_seq_cst); // all invalid
			word_access<const Struct> raw_value(initial_value);
			for(int i=0;i<STRUCT_SIZE;i++) {
				vals_[0].value[i].store<0>(0, memory_order_seq_cst);
				vals_[0].value[i].store<1>(raw_value[i], memory_order_seq_cst);
			}
		}

		big_int get_ver() {
			return current_version_.load<0>(memory_order_seq_cst);
		}

		bool publish(big_int old_version, big_int tid, const Struct& value) {
			word_access<const Struct> raw_value(value);
			version_tid current_version;
			current_version.version = old_version;
			if (current_version_.load<0>(memory_order_seq_cst) != old_version)
				return false;
			current_version.tid = current_version_.load<1>(memory_order_seq_cst);
			/* TODO
			 * The following check seems unnecessary. If an update has occured, to a version published by thread tid then tid's vals are all newer than old_version.
			 * Thus, help won't do a thing and will immediately return. Anyway, we will wish to abort the publish() then.
			 * 
			 * We need to check version first and load tid later! Otherwise we could happen on values from a failed publish!
			 */
			if (current_version_.load<0>(memory_order_seq_cst) != old_version)
				return false;
			help(current_version);
			/* TODO:
			 * First write (with relaxed mo) all versions. Then issue a barrier (what kind? -- we want all previous writes to appear to happen before subsequent ones)
			 * Then write all values, relaxedly. Then issue a barrier, prob. best by using the barrier from CAS (if you make the CAS seq_cst it should be all right)
			 */
			for(int i=0;i<STRUCT_SIZE;i++)
				vals_[tid].value[i].store<0>(old_version+1);
			for(int i=0;i<STRUCT_SIZE;i++)
				vals_[tid].value[i].store<1>(raw_value[i]);
			return current_version_.compare_exchange_strong(current_version.raw(), version_tid(old_version+1, tid).raw(), memory_order_seq_cst);
		}

		bool get(big_int version, Struct* dest)
		{
			{
				version_tid current_version;
				current_version.version = current_version_.load<0>(memory_order_seq_cst);
				current_version.tid = current_version_.load<1>(memory_order_seq_cst);
				if (current_version.version == current_version_.load<0>(memory_order_seq_cst))
					help(current_version);
			}
			word_access<Struct> dest_raw(*dest);
			for(int i=0;i<STRUCT_SIZE;i++) {
				history_element el = history_[version % H_].value[i].load(memory_order_seq_cst);
				if (el.version != version)
					return false;
				dest_raw[i] = el.value;
			}
			return true;
		}

		// TODO : get() returning a proxy that actually accesses the history for single member pointer derefs
};

