#ifndef __COUNTER_GUARD
#define __COUNTER_GUARD
#include "common.hpp"

namespace counter {
	enum {
		Parsum_Ops = 0,
		Parsum_Update,
		Parsum_OwnPublish,
		Parsum_UpdateLastCall,
		Parsum_UpdateLastExec,
		History_PublishCall,
		History_PublishSucc,
		N_Counter
	};

#ifndef RELACY
	intptr_t get_counter(int c);
	void finalize();
	void reset();
	void local_reset();
	void inc_counter(int c, intptr_t inc);
#else
	static inline void inc_counter(int c, intptr_t inc) { }
#endif
}

#endif	

