#include <cstring>
#include <algorithm>
#include <memory>

#include "util.hpp"
#include "large_atomic.hpp"

template<typename Struct> class history {
	private:
		static constexpr int Size = word_access<Struct>::Size;

		struct record {
			VAR_T(bool) should_save;
			std::atomic<word> version;
			std::atomic<word> value[Size];
		};

		const int H_;
		large_atomic<word, word> current_version_; // <version, tid>
		permuted_array<large_atomic<word, word> > history_[Size]; // [index][version modulo] <version, value> fixme -- make it back [verison][idx], now only one process will attempt writes of a version usually (is this sensible? we want to minimize get's cache invalidation probably)
		permuted_array<large_atomic<word, word> > history_tids_;
		std::unique_ptr<record[]> vals_; // [tid][index] <version, value>

		void save(word version, word tid)
		{
			LOG << "enter save(" << version << ", " << tid << ")";
			large_atomic<word, word>& elem = history_tids_[version%H_];
			word c_version = elem.load_1(std::memory_order_seq_cst);
			if (c_version >= version)
				return;
			word c_tid = elem.load_2(std::memory_order_seq_cst);
			std::pair<word, word> c_value(c_version, c_tid);
			elem.compare_exchange_strong(c_value, std::make_pair(version, tid), std::memory_order_seq_cst);
		}

		class get_proxy {
			private:
				word version_;
				word tid_;
				bool is_vals_newer_;
				history& history_;
				bool check_history_tids()
				{
					large_atomic<word, word>& tid_elem = history_.history_tids_[version_%history_.H_];
					if (tid_elem.load_1(std::memory_order_seq_cst) != version_)
						return false;
					tid_ = tid_elem.load_2(std::memory_order_seq_cst);
					if (unlikely(tid_elem.load_1(std::memory_order_seq_cst) != version_))
						throw too_old_exception(); // this means version we are seeking got overwritten between previous load and this one and is thus too old
					return true;
				}
				bool check_current_version()
				{
					large_atomic<word, word>& current_version = history_.current_version_;
					if (current_version.load_1(std::memory_order_seq_cst) != version_)
						return false;
					tid_ = current_version.load_2(std::memory_order_seq_cst);
					if (unlikely(current_version.load_1(std::memory_order_seq_cst) != version_))
						return false;
					return true;
				}
			public:
				get_proxy(history& h, word version) : history_(h), version_(version), is_vals_newer_(false)
				{
					if (!check_history_tids() && !check_current_version() && !check_history_tids())
						throw too_old_exception();
				}
				word operator[](int idx) {
					if (!is_vals_newer_) {
						word ret = history_.vals_[tid_].value[idx].load(std::memory_order_seq_cst);
						if (history_.vals_[tid_].version.load(std::memory_order_seq_cst) == version_)
							return ret;
						is_vals_newer_ = true;
					}
					large_atomic<word, word>& history_elem = history_.history_[idx][version_%history_.H_];
					word ret = history_elem.load_2(std::memory_order_seq_cst);
					if (history_elem.load_1(std::memory_order_seq_cst) != version_)
						throw too_old_exception();
					return ret;
				}
		};
	public:
		class too_old_exception {
		};

		history(int H, int T, const Struct& initial_value) : H_(H), history_tids_(H) {
			for(int i=0;i<Size;i++)
				history_[i] = permuted_array<large_atomic<word, word> >(H_);
			current_version_.store_1(2, std::memory_order_seq_cst);
			current_version_.store_2(0, std::memory_order_seq_cst);
			for(int i=0;i<Size;i++)
				for(int j=0;j<H_;j++)
					history_[i][j].store_1(i==0?1:0, std::memory_order_seq_cst); // all invalid
			word_access<const Struct> raw_value(initial_value);
			vals_.reset(new record[T]);
			for(int i=0;i<T;i++)
				VAR(vals_[i].should_save) = false;
			VAR(vals_[0].should_save) = true;
			vals_[0].version.store(2, std::memory_order_seq_cst);
			for(int i=1;i<T;i++)
				vals_[i].version.store(0, std::memory_order_seq_cst);
			for(int i=0;i<Size;i++)
				vals_[0].value[i].store(raw_value[i], std::memory_order_seq_cst);
		}

		word get_ver() {
			return current_version_.load_1(std::memory_order_seq_cst);
		}

		bool publish(word old_version, word tid, const Struct& value) {
			word_access<const Struct> raw_value(value);
			if (current_version_.load_1(std::memory_order_seq_cst) != old_version)
				return false;
			word my_previous_version = vals_[tid].version.load(std::memory_order_seq_cst);
			if (VAR(vals_[tid].should_save) && old_version <= my_previous_version + H_) {
				for(int i=0;i<Size;i++) {
					word val = vals_[tid].value[i].load(std::memory_order_seq_cst);
					large_atomic<word, word>& hist_entry =  history_[i][my_previous_version%H_];
					std::pair<word, word> prev;
					prev.first = hist_entry.load_1(std::memory_order_seq_cst);
					assert(prev.first != my_previous_version);
					if (prev.first > my_previous_version)
						break;
					prev.second = hist_entry.load_2(std::memory_order_seq_cst);
					if (!hist_entry.compare_exchange_strong(prev, std::make_pair(my_previous_version, val), std::memory_order_seq_cst))
						break;
				}
			}
			VAR(vals_[tid].should_save) = false;
			vals_[tid].version.store(old_version+1, std::memory_order_release);
			for(int i=0;i<Size;i++)
				vals_[tid].value[i].store(raw_value[i], std::memory_order_release);
			word old_tid = current_version_.load_2(std::memory_order_seq_cst);
			if (current_version_.load_1(std::memory_order_seq_cst) != old_version)
				return false;
			save(old_version, old_tid);
			std::pair<word, word> old_current_version(old_version, old_tid);
			if (!current_version_.compare_exchange_strong(old_current_version, std::make_pair(old_version+1, tid), std::memory_order_seq_cst))
				return false;
			VAR(vals_[tid].should_save) = true;
			save(old_version+1, tid);
		}

		typedef word_proxy<Struct, get_proxy> proxy;

		proxy get(word version) {
			return word_proxy<Struct, get_proxy>(get_proxy(*this, version));
		}

};

