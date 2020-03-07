#pragma once
#include <atomic>
#include <memory>
#include <random>
#include "mrlock.h"

struct casRow {
	long *address;
	long expectedValue;
	long newValue;
	uint32_t resId;
	uint32_t handle;
};

class mrlock_mcas
{
public:
	inline static void intial(uint32_t res)
	{
		m = new MRLock<uint64_t>(res*2);
	}

	inline static void complete(casRow* start, casRow* last,bool success=true)
	{
		for (casRow* runner = start; runner <= last; runner++)
		{
			if(success)
				(*runner->address) = runner->newValue;
			m->Unlock(runner->handle);
		}
	}

	inline static void invokeMcas(casRow* start, casRow* last)
	{
		
		for (casRow* runner = start; runner <= last; runner++)
		{
			runner->handle = m->Lock(runner->resId);
			if (*runner->address != runner->expectedValue)
			{
				complete(start, runner, false);
				return;
			}
		}
		//at this point all are equal, modify them to the new values
		complete(start, last);
	}

private:
	static MRLock<uint64_t>* m;
};