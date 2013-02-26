#include "util.hpp"
#include "holder.hpp"
#include "history.hpp"

#include <string>
using std::string;

template<typename T> class reducing_snapshot {
	public:
		struct update_result {
			intptr_t update_count;
			intptr_t version;
			T value_before;
		};

		constexpr static int INVALID_VERSION = -1;

		virtual intptr_t get_version() = 0;
		virtual intptr_t get(T* output) = 0;
		virtual update_result update(intptr_t tid, const T* input) = 0;
		virtual bool get_last(intptr_t tid, update_result* output) = 0;
		virtual ~reducing_snapshot() { }

		static reducing_snapshot* create(int thread_count);
};

typedef raw_locked_history RawHistory;
typedef raw_locked_holder RawHolder;

template<typename T> class leaf : public reducing_snapshot<T> {
	private:
		typedef typename reducing_snapshot<T>::update_result update_result;
		constexpr static int INVALID_VERSION = reducing_snapshot<T>::INVALID_VERSION;

		history<T, RawHistory> history_;
	public:
		virtual intptr_t get_version() {
			return history_.get_version();
		}

		virtual intptr_t get(T* output) {
			intptr_t version = history_.get_version();
			if (!history_.get(version, output))
				return INVALID_VERSION;
			return version;
		}

		virtual update_result update(intptr_t tid, const T* input) {
			RL_ASSERT(tid == 0);
			update_result result;
			intptr_t version = history_.get_version() + 1;
			bool ok = history_.publish(0, version, input);
			RL_ASSERT(ok);
			ok = get_last(0, &result);
			RL_ASSERT(ok);
			return result;
		}

		virtual bool get_last(intptr_t tid, update_result* output) {
			RL_ASSERT(tid == 0);
			intptr_t version = history_.get_version();
			output->update_count = version;
			output->version = version;
			return history_.get(version-1, &output->value_before);
		}

		leaf() : history_(3, 1) {}
};

template<typename T> class node : public reducing_snapshot<T> {
	private:
		typedef typename reducing_snapshot<T>::update_result update_result;
		constexpr static int INVALID_VERSION = reducing_snapshot<T>::INVALID_VERSION;

		struct value {
			T child_values[2];
			intptr_t child_versions[2];
		};
		struct latest_value {
			intptr_t version;
			T value_before;
		};

		const int thread_count_;
		
		history<value, RawHistory> history_;
		std::unique_ptr<holder<latest_value, RawHolder>[]> latest_;
		std::unique_ptr<reducing_snapshot<T> > children_[2];

		int child_idx(int tid) { return tid % 2; }
		int child_tid(int tid) { return tid / 2; }

		void update_last(intptr_t tid) {
			update_result child_result;
			bool ok = children_[child_idx(tid)]->get_last(child_tid(tid), &child_result);
			if (!ok)
				return;
			intptr_t version_after = history_.get_version();
			intptr_t version_before = version_after - 3*thread_count_;
			while (version_before + 1 < version_after) {
				intptr_t middle = (version_before + version_after) / 2;
				value v;
				ok = history_.get(middle, &v);
				if (ok && v.child_versions[child_idx(tid)] >= child_result.version)
					version_after = middle;
				else
					version_before = middle;
			}
			RL_ASSERT(version_before + 1 == version_after);
			value value_before, value_after;
			if (!history_.get(version_before, &value_before))
				return;
			if (!history_.get(version_after, &value_after))
				return;
			if (value_after.child_versions[child_idx(tid)] < child_result.version)
				return;
			latest_value lv;
			lv.version = version_after;
			if (child_idx(tid) == 0)
				lv.value_before = child_result.value_before + value_before.child_values[1];
			else
				lv.value_before = value_after.child_values[0] + child_result.value_before ;
			//LOG << "N" << name_ << " update_last tid=" << tid << " val_before=" << lv.value_before << " version=" << lv.version << " update_count=" << child_result.update_count;
			latest_[tid].update(child_result.update_count, &lv);
		}
	public:
		virtual intptr_t get_version() {
			return history_.get_version();
		}

		virtual intptr_t get(T* result) {
			intptr_t version = history_.get_version();
			value buffer;
			if (!history_.get(version, &buffer))
				return INVALID_VERSION;
			*result = buffer.child_values[0] + buffer.child_values[1];
			return version;
		}

		virtual update_result update(intptr_t tid, const T* val) {
			update_result child_result = children_[child_idx(tid)]->update(child_tid(tid), val);
			intptr_t other_version = children_[1-child_idx(tid)]->get_version();

			do {
				intptr_t my_version = get_version();
				value my_value;
				if (!history_.get(my_version, &my_value))
					break;
				if (my_value.child_versions[child_idx(tid)] >= child_result.version && my_value.child_versions[1-child_idx(tid)] >= other_version)
					break;
				value new_value;
				new_value.child_versions[0] = children_[0]->get(&new_value.child_values[0]);
				new_value.child_versions[1] = children_[1]->get(&new_value.child_values[1]);
				if (new_value.child_versions[0] == INVALID_VERSION || new_value.child_versions[1] == INVALID_VERSION)
					break;
				update_last(my_version % thread_count_);
				if (history_.publish(tid, my_version+1, &new_value)) {
			//		LOG << "N" << name_ << "succ publish v="<<my_version+1 << " chvers=" << new_value.child_versions[0] << " " << new_value.child_versions[1];
					break;
				}
			} while (true);

			update_result result;
			bool ok = get_last(tid, &result);
			RL_ASSERT(ok);
			return result;
		}

		virtual bool get_last(intptr_t tid, update_result* output) {
			update_last(tid);
			latest_value last_value;
			intptr_t last_version = latest_[tid].get(&last_value);
			if (last_version == holder<latest_value>::INVALID_VERSION)
				return false;
			output->update_count = last_version;
			output->value_before = last_value.value_before;
			output->version = last_value.version;
			return true;
		}

	
		node(int thread_count) :
			thread_count_(thread_count),
			history_(3*thread_count, thread_count),
			latest_(new holder<latest_value, RawHolder>[thread_count])
		{
			RL_ASSERT(thread_count_ > 1);
			children_[0].reset(reducing_snapshot<T>::create((thread_count + 1) / 2));
			children_[1].reset(reducing_snapshot<T>::create(thread_count / 2));
		}
};

template<typename T> reducing_snapshot<T>* reducing_snapshot<T>::create(int thread_count) {
	RL_ASSERT(thread_count > 0);
	if (thread_count == 1)
		return new leaf<T>();
	else
		return new node<T>(thread_count);
}

