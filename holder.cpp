#include "holder.hpp"

using rl::mo_seq_cst;

using std::make_pair;

intptr_t raw_holder::get(intptr_t* value) {
	intptr_t version = contents_[0].load_1(memory_order_seq_cst);
	for(int i=0;i<element_size_;i++) {
		if (contents_[i].load_1(memory_order_seq_cst) != version)
			return INVALID_VERSION;
		value[i] = contents_[i].load_2(memory_order_seq_cst);
		if (contents_[i].load_1(memory_order_seq_cst) != version)
			return INVALID_VERSION;
	}
	return version;
}

void raw_holder::update(intptr_t version, const intptr_t* value) {
	for(int i=0;i<element_size_;i++) {
		intptr_t previous_version = contents_[i].load_1(memory_order_seq_cst);
		if (previous_version >= version)
			continue;
		RL_ASSERT(previous_version == version - 1);
		intptr_t previous_data = contents_[i].load_2(memory_order_seq_cst);
		contents_[i].compare_exchange_strong(make_pair(previous_version, previous_data), make_pair(version, value[i]), memory_order_seq_cst);
	}
}

raw_holder::raw_holder(int element_size) :
	element_size_(element_size),
	contents_(new large_atomic<intptr_t, intptr_t>[element_size])
{
}

intptr_t raw_locked_holder::get(intptr_t* value) {
	m_.lock($);
	for(int i=0;i<element_size_;i++)
		value[i] = VAR(contents_[i]);
	intptr_t ver = VAR(version_);
	m_.unlock($);
	return ver;
}

void raw_locked_holder::update(intptr_t version, const intptr_t* value) {
	m_.lock($);
	if (VAR(version_) == version) {
		for(int i=0;i<element_size_;i++)
			RL_ASSERT(value[i] == VAR(contents_[i]));
	}
	if (VAR(version_) < version) {
		RL_ASSERT(VAR(version_) == version - 1);
		VAR(version_) = version;
		for(int i=0;i<element_size_;i++)
			VAR(contents_[i]) = value[i];
	}
	m_.unlock($);
}

raw_locked_holder::raw_locked_holder(int element_size) :
	element_size_(element_size),
	contents_(new VAR_T(intptr_t)[element_size]),
	version_(0)
{
	for(int i=0;i<element_size_;i++)
		VAR(contents_[i]) = 0;
}
