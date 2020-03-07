#pragma once
#include <atomic>
#include <memory>
#include "mrlock.h"

struct casRow {
	long *address;
	long expectedValue;
	long newValue;
	uint32_t handle;
};

class seq_mcas
{
public:
	inline static void complete(casRow* start, casRow* last, MRLock<casRow*>& m)
	{
		for (casRow* runner = start; runner <= last; runner++)
		{
			(*runner->address) = runner->newValue;
			m.Unlock(runner->handle);
		}
	}

	inline static void invokeMcas(casRow* start, casRow* last)
	{
		MRLock<casRow*> m = MRLock<casRow*>(0x1);


		for (casRow* runner = start; runner<=last; runner ++)
		{
			runner->handle = m.Lock(runner);
			if (*runner->address != runner->expectedValue)
				return;
		}
		//at this point all are equal, modify them to the new values
		complete(start, last,m);
	}
};