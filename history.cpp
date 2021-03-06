#include "history.hpp"

USING_MEMORY_ORDERS;

using std::make_pair;

bool raw_history::get_from_latest(version_t ver, tid_t creator, data_t* output) {
	// Precondition: Version ver was already published by thread creator
	for(int i = 0; i < element_size_; i++) {
		output[i] = latest_[creator].data(i).load(memory_order_seq_cst); // acq
	}
	return latest_[creator].version().load(memory_order_seq_cst) == ver; //relaxed
}

bool raw_history::get_from_buffer(version_t ver, data_t* output) {
	for(int i = 0; i < element_size_; i++)
		output[i] = buffer_[ver % history_size_][i].load_2(memory_order_seq_cst); // acq
	return buffer_[ver % history_size_][0].load_1(memory_order_seq_cst) == ver; //relaxed
}

bool raw_history::get(version_t ver, data_t* output) {
	if (ver < 0)
		return false;
	version_t current_version = current_version_.load_1(memory_order_seq_cst); //acq
	if (ver > current_version || ver <= current_version - history_size_)
		return false;
	tid_t creator;
	bool check_latest = false;
	if (ver == current_version) {
		creator = current_version_.load_2(memory_order_seq_cst); //acq
		if (current_version_.load_1(memory_order_seq_cst) == ver) //relaxed
			check_latest = true;
	}
	if (check_latest) {
		if (get_from_latest(ver, creator, output))
			return true;
	}
	return get_from_buffer(ver, output);
}

bool raw_history::publish(tid_t me, version_t ver, const data_t* input) {
	assert(me < thread_count_);
	if (ver < 0)
		return false;
	version_t previous_version = current_version_.load_1(memory_order_seq_cst);
	if (ver != previous_version + 1)
		return false;
	tid_t previous_creator = current_version_.load_2(memory_order_seq_cst);
	// TODO: Deal with ordering issues between different words of the element.
	// We want to be able to tell that all words of the previous version were
	// stored. For now, we check all of them.
	data_t previous_element[element_size_];
	bool previous_read = false;
	for(int i=0;i<element_size_;i++) {
		version_t buffer_version = buffer_[previous_version % history_size_][i].load_1(memory_order_seq_cst);
		if (buffer_version >= previous_version)
			continue;
		if (!previous_read) {
			if (!get_from_latest(previous_version, previous_creator, previous_element))
				break;
			previous_read = true;
		}
		data_t buffer_data = buffer_[previous_version % history_size_][i].load_2(memory_order_seq_cst);
		std::pair<version_t, data_t> expected(buffer_version, buffer_data);
		buffer_[previous_version % history_size_][i].compare_exchange_strong(expected, make_pair(previous_version, previous_element[i]), memory_order_seq_cst);
	}
	latest_[me].version().store(ver, memory_order_relaxed);
	std::atomic_thread_fence(memory_order_release);
	for(int i=0;i<element_size_;i++)
		latest_[me].data(i).store(input[i], memory_order_relaxed);
	std::pair<version_t, tid_t> previous_cv(previous_version, previous_creator);
	return current_version_.compare_exchange_strong(previous_cv, make_pair(ver, me), memory_order_seq_cst);
}

raw_history::raw_history(int element_size, int history_size, int thread_count) :
	element_size_(element_size),
	history_size_(history_size),
	thread_count_(thread_count),
	current_version_(make_pair(0, 0)),
	buffer_(new std::unique_ptr<large_atomic<version_t, data_t>[]>[history_size]),
	latest_(new latest_t[thread_count])
{
	for(int i=0;i<history_size;i++) {
		buffer_[i].reset(new large_atomic<version_t, data_t>[element_size]);
		// All elements get initialized to (0, 0).
	}
	for(int i=0;i<thread_count;i++) {
		latest_[i].contents.reset(new ATOMIC(uintptr_t)[element_size+1]);
		latest_[i].version().store(0, std::memory_order_seq_cst);
		for(int j=0;j<element_size;j++)
			latest_[i].data(j).store(0, std::memory_order_seq_cst);
	}
}

raw_locked_history::version_t raw_locked_history::get_version() {
	m_.lock($);
	intptr_t version = VAR(current_version_);
	m_.unlock($);
	return version;
}

bool raw_locked_history::get(version_t ver, data_t* output) {
	if (ver < 0)
		return false;
	m_.lock($);
	if (ver > VAR(current_version_) || ver <= VAR(current_version_) - history_size_) {
		m_.unlock($);
		return false;
	}
	for(int i=0;i<element_size_;i++)
		output[i] = VAR(buffer_[ver % history_size_][i]);
	m_.unlock($);
	return true;
}

bool raw_locked_history::publish(tid_t me, version_t ver, const data_t* input) {
	VAR(thread_checks_[me]) = true;
	if (ver < 0)
		return false;
	m_.lock($);
	bool ok = false;
	if (VAR(current_version_) + 1 == ver) {
		ok = true;
		VAR(current_version_) = ver;
		for(int i=0;i<element_size_;i++)
			VAR(buffer_[ver % history_size_][i]) = input[i];
	}
	m_.unlock($);
	VAR(thread_checks_[me]) = true;
	return ok;
}

raw_locked_history::raw_locked_history(int element_size, int history_size, int thread_count):
	element_size_(element_size),
	history_size_(history_size),
	thread_count_(thread_count),
	thread_checks_(new VAR_T(bool)[thread_count]),
	buffer_(new std::unique_ptr<VAR_T(intptr_t)[]>[history_size]),
	current_version_(0)
{
	for(int i=0;i<history_size_;i++) {
		buffer_[i].reset(new VAR_T(intptr_t)[element_size_]);
		for(int j=0;j<element_size_;j++)
			VAR(buffer_[i][j]) = 0;
	}
}
