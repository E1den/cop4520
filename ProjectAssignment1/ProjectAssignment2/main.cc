// Tim O'Brien Andrew Kiner
// COP 4520 Project Assignment 2
// Based off A Practical Wait-Free Multi-Word Compare-And-Swap
#include <iostream>
#include <stack>
#include <thread>
#include <atomic>
#include <vector>
#include <bitset>

using namespace std;

#define nThreads 1
#define numThreads nThreads
#define maxFail 5
uintptr_t pendingOpTable[nThreads];

thread_local int32_t threadId, checkId, recurDepth;
thread_local uintptr_t tl_op;

//BitMask
constexpr std::uint_fast8_t mask0{ 0b0000'0001 };

// Samples
int32_t _on = 1;
int32_t _tw = 2;
int32_t _th = 3;

std::atomic<int32_t*>* one = new std::atomic<int32_t*>(&_on);
std::atomic<int32_t*>* two = new std::atomic<int32_t*>(&_tw);
std::atomic<int32_t*>* three = new std::atomic<int32_t*>(&_th);

class ReferenceCount
{
public:
	std::atomic_int count;

	ReferenceCount()
	{
		count.store(1);
	}

	void increment()
	{
		count++;
	}

	void decrement()
	{
		count--;
		if (count.load() == 0)
			delete this;
	}
};

template<typename T>
class MCasHelper;
template<typename T>
class CasRow;
template <typename T>
void helpIfNeeded();

void SafeFree(uintptr_t t)
{
	if (t != 0)
		delete reinterpret_cast<void*>(t);
}


// Thread local values
// threadID
// checkID
// recurDepth
// tl_op
template <typename T>
class MCasHelper : public ReferenceCount
{
public:
	CasRow<T>* cr;
	MCasHelper(CasRow<T>* curr)
	{
		//ReferenceCount();
		cr = curr;
	}
	~MCasHelper()
	{
		decrement();
	}
};

template <typename T>
class CasRow
{
public:
	atomic<T*>* address;
	T expectedValue;
	T newValue;
	atomic<MCasHelper<T>*>* mch;
	CasRow(atomic<T*>* addr, T eValue, T nValue, atomic<MCasHelper<T>*>* help)
	{
		address = addr;
		expectedValue = eValue;
		newValue = nValue;
		mch = help;
	}
};

template <class T>
struct MCasDescriptor
{
	CasRow<T>* myRow;
	int myInt = 0x1;
	MCasDescriptor(CasRow<T>* row)
	{
		myRow = row;
	}
};

template <class T>
T CAS1(atomic<T>* addr, T* oldval, T* newval)
{
	uintptr_t old = reinterpret_cast<uintptr_t>(addr);
	addr->compare_exchange_strong(oldval, newval);
	return old;
}

template<typename T>
uintptr_t bitmask(MCasHelper<T>* d)
{
	uintptr_t p = reinterpret_cast<uintptr_t>(d);
	p |= mask0;
	return p;
}

template<typename T>
MCasHelper<T>* unmask(uintptr_t d)
{
	return reinterpret_cast<MCasHelper<T>*>(d &= ~mask0);
}

bool isMCasHelper(uintptr_t d)
{
	if ((d & mask0) != 0) return true;
	return false;
}

template<class T>
uintptr_t allocateMCasHelper(CasRow<T>* mcasp)
{
	uintptr_t p = reinterpret_cast<uintptr_t>(mcasp);
	p |= mask0;
	return p;
}

template <class T>
bool invokeMCAS(CasRow<T>* mcasp, CasRow<T>* lastRow)
{
	//==thread_local int threadID = std::this_thread::get_id
	helpIfNeeded<T>();
	//tl_op = MCasHelper<T>(mcasp);
	tl_op = allocateMCasHelper(mcasp);
	placeMCasHelper(mcasp++, lastRow, true);
	while (mcasp != lastRow)
	{
		if (lastRow->mch == 0x0)
		{
			placeMCasHelper(mcasp++, lastRow, false);
		}
		else
			break;
	}
	pendingOpTable[threadId] = 0;
	bool res = (reinterpret_cast<uintptr_t>(lastRow->mch) != ~0x0);
	removeMCasHelper(res, mcasp, lastRow);
	return res;

}

/*template class<T>
//bool rcWatch(MCasHelper* cValue, T** address)
//{
	T old = cValue->expectedValue;
// if(cValue.count == 1)
	{
		cValue.increment();

		if(old == **address)
		{
			return true;
		}
//		else
//		{
//			return false;
//		}
	}
//}
*/
template <typename T>
bool rcWatch(MCasHelper<T>* cValue, T* address)
{
	T o = cValue->cr->expectedValue;
	cValue->increment();
	if (o == *address)
		return true;
	else
		return false;
}

template <typename T>
void rcUnWatch(MCasHelper<T>* value)
{
	value->decrement();
}

template <typename T>
void placeMCasHelper(CasRow<T>* cr, CasRow<T>* lastRow, bool firstTime)
{
	std::atomic<T*> address = (cr->address->load());
	bool res;
	MCasHelper<T>* mch = cr->mch->load();//reinterpret_cast<MCasHelper<T>*>(tl_op);
	T eValue = cr->expectedValue;
	if (firstTime)
	{
		cr->mch->store(mch);
		mch = reinterpret_cast<MCasHelper<T>*>(tl_op);
	}
	else
	{
		mch = reinterpret_cast<MCasHelper<T>*>(allocateMCasHelper(cr));
	}
	T* cValue = address.load();
	int32_t tries = 0;
	while (firstTime || cr->mch == NULL)
	{
		if (tries++ == maxFail)// && pendingOpTable[threadID] == null)
		{
			if (firstTime)
			{
				cr->mch = nullptr;
				firstTime = false;
			}
			pendingOpTable[threadId] = tl_op;
			if (recurDepth > 0)
			{
				// FULL Return to its own operation
				/*Full */return;
			}
		}
		if (!isMCasHelper(reinterpret_cast<uintptr_t>(cValue)))
		{
			cValue = address.load();
			res = address.compare_exchange_strong(cValue, reinterpret_cast<T*>(mch));
			/*if (firstTime)
			{
				cr->mch = mch;
				return;
			}*/
			//cv - address.compare_exchange_weak(ev, mch)
			if (*cValue == eValue)
			{
				if (!firstTime)
				{
					MCasHelper<T>* tmp = nullptr;
					cValue = reinterpret_cast<T*>(cr->mch->load());
					cr->mch->compare_exchange_strong(tmp, mch);
					//cValue = CAS1(cr->mch, tmp, mch);
					if (cValue != nullptr && cValue != reinterpret_cast<T*>(mch))
					{
						T* tmp2 = reinterpret_cast<T*>(mch);
						address.compare_exchange_strong(tmp2, &eValue);
						//safeFree(mch);
					}
				}
				return;
			}
			else
			{
				continue;
			}
		}
		else
		{
			if (!rcWatch<T>(reinterpret_cast<MCasHelper<T>*>(cValue), address.load()))
			{
				continue;
			}
			if (cr == reinterpret_cast<MCasHelper<T>*>(cValue)->cr)
			{
				delete mch;
				MCasHelper<T>* tmp = nullptr;
				T* cValue2 = reinterpret_cast<T*>(cr->mch->load());
				cr->mch->compare_exchange_strong(tmp, reinterpret_cast<MCasHelper<T>*>(cValue));
				if (cValue2 != NULL && cValue2 != cValue)
				{
					address.compare_exchange_strong(cValue, &eValue);
				}
				rcUnWatch<T>(reinterpret_cast<MCasHelper<T>*>(cValue));
				return;
			}
			else if (shouldReplace(&eValue, reinterpret_cast<MCasHelper<T>*>(cValue)))
			{
				T* cValue2 = reinterpret_cast<T*>(cr->mch->load());
				address.compare_exchange_strong(cValue, reinterpret_cast<T*>(mch));

				if (cValue2 == cValue)
				{
					if (!firstTime)
					{
						MCasHelper<T>* tmp = nullptr;
						cr->mch->compare_exchange_weak(tmp, mch);
						cValue = reinterpret_cast<T*>(cr->mch);
						if (cValue != NULL && cValue != reinterpret_cast<T*>(mch))
						{
							T* tmp = reinterpret_cast<T*>(mch);
							address.compare_exchange_strong(tmp, &eValue);
							SafeFree(reinterpret_cast<uintptr_t>(mch));
						}
					}
					rcUnWatch<T>(reinterpret_cast<MCasHelper<T>*>(cValue));
					return;
				}
				else
				{
					rcUnWatch<T>(reinterpret_cast<MCasHelper<T>*>(cValue));
					continue;
				}
			}


		}

		MCasHelper<T>* tmp = nullptr;
		res = cr->mch->compare_exchange_strong(tmp, reinterpret_cast<MCasHelper<T>*>(~0x0));
		if (res == NULL)
		{
			lastRow->mch->compare_exchange_strong(tmp, reinterpret_cast<MCasHelper<T>*>(~0x0));
		}
		free(mch);
		return;
	}
}

template <typename T>
bool shouldReplace(T* ev, MCasHelper<T>* mch)
{
	CasRow<T>* cr = mch->cr;
	if (cr->expectedValue != *ev && cr->newValue != *ev)
	{
		return false;
	}
	else
	{
		int32_t res = helpComplete(mch);
		if (res && (cr->mch->load() == mch))
		{
			if (cr->newValue == *ev)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			if (cr->expectedValue == *ev)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
	}
}

template <typename T>
void helpIfNeeded()
{
	checkId = (checkId + 1) % nThreads;
	tl_op = pendingOpTable[checkId];
	if (tl_op != NULL && rcWatch(reinterpret_cast<MCasHelper<T>*>(tl_op), reinterpret_cast<T*>(pendingOpTable[checkId])))
	{
		if (isMCasHelper(tl_op))
		{
			//helpComplete(reinterpret_cast<MCasHelper<T>*>(pendingOpTable[checkId]),0);
			helpComplete(reinterpret_cast<MCasHelper<T>*>(pendingOpTable[checkId]));
		}
		else
		{
			//mcasRead(tl_op->address, false);
		}
		rcUnWatch<T>(reinterpret_cast<MCasHelper<T>*>(tl_op));
	}
}

template<typename T>
int32_t helpComplete(MCasHelper<T>* mch)
{
	recurDepth++;
	if (recurDepth > nThreads)
	{
		/*Full*/ return -1;
	}

	CasRow<T>* cr = (mch->cr + 1);
	CasRow<T>* lastRow = cr;
	while (lastRow->address->load() != reinterpret_cast<T*>(0x1))
	{
		lastRow++;
	}
	lastRow--;
	while (cr != lastRow)
	{
		if (lastRow->mch == 0x0)
		{
			placeMCasHelper(cr++, lastRow, false);
			if (cr->mch->load() == reinterpret_cast<MCasHelper<T>*>(~0x0))
			{
				break;
			}
		}
		else
		{
			break;
		}
	}
	return (lastRow->mch->load() != reinterpret_cast<MCasHelper<T>*>(~0x0));
}

template<typename T>
void removeMCasHelper(bool passed, CasRow<T>* m, CasRow<T>* lastRow)
{
	while (m != lastRow)
	{
		T* tmp = reinterpret_cast<T*>(m->mch->load());
		if (reinterpret_cast<uintptr_t>(m->mch) == ~0x0)
		{
			return;
		}
		else if (passed)
		{
			m->address->compare_exchange_strong(tmp, reinterpret_cast<T*>(m->newValue));
		}
		else
		{
			m->address->compare_exchange_strong(tmp, reinterpret_cast<T*>(m->expectedValue));
		}
		SafeFree(reinterpret_cast<uintptr_t>(m->mch->load()));
		m++;
	}
}

template <class T>
T* mcasRead(atomic<T>** address, bool myOp = true)
{
	T* cValue = *address;
	if (myOp)
	{
		tl_op = reinterpret_cast<uintptr_t>(*reinterpret_cast<void**>(tl_op));//new ReadOp(address); //wtf does ReadOp do?
	}
	int tries = 0;
	while (true)
	{
		if (tl_op->value != NULL)
		{
			cValue = tl_op->value;
			pendingOpTable[threadId] = NULL;
			if (myOp)
			{
				SafeFree(tl_op);
			}
			return cValue;
		}
		if (tries++ == maxFail)
		{
			pendingOpTable[threadId] = tl_op;
		}
		if (!isMCasHelper(cValue))
		{
			if (pendingOpTable[threadId] == NULL)
			{
				SafeFree(tl_op);
			}
			else
			{
				&tl_op->value.compare_exchange_weak(NULL, cValue);
				pendingOpTable[threadId] = NULL;
				if (myOp)
				{
					SafeFree(tl_op);
				}
			}
			return cValue;
		}
		else
		{
			if (!rcWatch(cValue, address))
			{
				continue;
			}
			if (cValue->cr == NULL || cValue->cr == ~0x0)
			{
				cValue = cValue->cr->expectedValue;
			}
			else
			{
				bool res = helpComplete(cValue);
				if (res && cValue->cr == cValue--)
				{
					cValue = cValue->cr->newValue;
				}
				else
				{
					cValue = cValue->cr->expectedValue;
				}
			}
			if (pendingOpTable[threadId] == NULL)
			{
				SafeFree(tl_op);
			}
			else
			{
				T* tmp = nullptr;
				reinterpret_cast<T*>(tl_op)->value.compare_exchange_strong(tmp, cValue);
				pendingOpTable[threadId] = NULL;
				if (myOp)
				{
					SafeFree(tl_op);
				}
			}
			rcUnWatch(cValue);
			return cValue;
		}
	}
}

template <typename T>
bool mcasCAS(void** address, T* expected, T* newV, bool MyOp = true)
{
	MCasHelper<T>* cValue = *address;
	T lcValue;
	uint32_t tries = 0;
	bool res;
	while (tries++ < maxFail)
	{
		if (!isMCasHelper(cValue))
		{
			if (cValue.load() == expected)
			{
				res = address.compare_exchange_strong(expected, newV);
				return res;
			}
			else
			{
				continue;
			}
		}
		else
		{
			if (!rcWatch(cValue, address))
			{
				continue;
			}
			if (cValue->cr == NULL || cValue->cr == ~0x0)
			{
				lcValue = cValue->cr->expectedValue;
			}
			else
			{
				res = helpComplete(cValue);
				if (res && cValue->cr == cValue--)
				{
					lcValue = cValue->cr->newValue;
				}
				else
				{
					lcValue = cValue->cr->expectedValue;
				}
			}
			if (lcValue == expected)
			{
				res = address.compare_exchange_weak(cValue, newV);
				rcUnWatch(cValue);
				return res;
			}
			else
			{
				rcUnWatch(cValue);
				continue;
			}
		}
		MCasDescriptor<T>* mcas = new MCasDescriptor<T>(1, address, expected);
		bool res = invokeMCAS(mcas->first, mcas->last);
		safeFree(mcas);
		return res;
	}
}






// Function for threads to run
void myFunction(int32_t id)
{
	threadId = id;
	checkId = 0;
	recurDepth = 0;
	tl_op = 0;

	CasRow<int32_t>* myArr = (CasRow<int32_t>*)malloc(sizeof(CasRow<int32_t>) * 3);
	//needs to reference the pointer of the new row, needs to be seperate lines or will reference the old values in the array
	myArr[0] = CasRow<int32_t>(one, 1, 37, nullptr);
	myArr[0].mch = new atomic<MCasHelper<int32_t>*>(new MCasHelper<int32_t>(myArr));

	myArr[1] = CasRow<int32_t>(two, 2, 38, nullptr);
	myArr[1].mch = new atomic<MCasHelper<int32_t>*>(new MCasHelper<int32_t>(myArr+1));

	myArr[2] = CasRow<int32_t>(three, 3, 39, nullptr);
	myArr[2].mch = new atomic<MCasHelper<int32_t>*>(new MCasHelper<int32_t>(myArr+2));

	invokeMCAS(myArr, &myArr[2]);
}



// Main function
int main(int argc, char** argv)
{
	std::vector<std::thread> threads = std::vector<std::thread>();
	for (auto i = 0; i < numThreads; i++)
		threads.push_back(std::thread([i] {myFunction(i); }));

	for (auto j = 0; j < numThreads; j++)
		threads[j].join();
}