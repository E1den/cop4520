#include <iostream>
#include <thread>
#include "mrlock_mcas.h"

MRLock<uint64_t>* mrlock_mcas::m = nullptr;

int main(int argc, char** argv)
{
	long one = 1;
	long two = 2;
	long three = 3;

	casRow arr[3];

	arr[0] = {};
	arr[0].address = &one;
	arr[0].expectedValue = 1;
	arr[0].newValue = 37;
	arr[0].resId = 0b1;

	arr[1] = {};
	arr[1].address = &two;
	arr[1].expectedValue = 2;
	arr[1].newValue = 38;
	arr[0].resId = 0b10;

	arr[2] = {};
	arr[2].address = &three;
	arr[2].expectedValue = 3;
	arr[2].newValue = 39;
	arr[0].resId = 0b100;

	mrlock_mcas::intial(sizeof(arr) / sizeof(casRow));

	const uint32_t numThreads = 4;

	std::thread threads[numThreads];

	for (auto i = 0; i < numThreads; i++)
		threads[i] = std::thread([&] {
		for (;;) {
			std::cout << "Before: " << one << ", " << two << ", " << three << std::endl;
			mrlock_mcas::invokeMcas(arr, &arr[2]);
			std::cout << "After: " << one << ", " << two << ", " << three << std::endl;
		}});

	for (auto i = 0; i < numThreads; i++)
		threads[i].join();

	return 0;
}