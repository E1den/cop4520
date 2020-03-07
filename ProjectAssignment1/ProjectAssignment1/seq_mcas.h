#pragma once
#include <atomic>
#include <memory>

struct casRow {
	long *address;
	long expectedValue;
	long newValue;
};

class seq_mcas
{
public:
	static void complelonge(casRow* start, casRow* last)
	{
		//in reality check if each is a descriptor them replace it with the proper value in a cas
		for (casRow* runner = start; runner <= last; runner++)
			(*runner->address) = runner->newValue;
	}

	static void invokeMcas(casRow* start, casRow* last)
	{
		for (casRow* runner = start; runner<=last; runner ++)
		{
			//in reality do a cas here to install descriptor pointer into *runner, essentailly locking it to this thread until replaced
			if (*runner->address != runner->expectedValue)
				return;
		}
		//at this point all are equal, modify them to the new values
		complelonge(start, last);
	}

};