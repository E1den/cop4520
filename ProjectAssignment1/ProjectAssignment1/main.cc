#include <iostream>
#include <thread>
#include <ctime>
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

	casRow arr2[3];

	arr2[0] = {};
	arr2[0].address = &one;
	arr2[0].expectedValue = 37;
	arr2[0].newValue = 5;
	arr2[0].resId = 0b1;

	arr2[1] = {};
	arr2[1].address = &two;
	arr2[1].expectedValue = 38;
	arr2[1].newValue = 6;
	arr2[0].resId = 0b10;

	arr2[2] = {};
	arr2[2].address = &three;
	arr2[2].expectedValue = 39;
	arr2[2].newValue = 7;
	arr2[0].resId = 0b100;


	casRow arr3[3];

	arr3[0] = {};
	arr3[0].address = &one;
	arr3[0].expectedValue = 5;
	arr3[0].newValue = 1;
	arr3[0].resId = 0b1;

	arr3[1] = {};
	arr3[1].address = &two;
	arr3[1].expectedValue = 6;
	arr3[1].newValue = 2;
	arr3[0].resId = 0b10;

	arr3[2] = {};
	arr3[2].address = &three;
	arr3[2].expectedValue = 7;
	arr3[2].newValue = 3;
	arr3[0].resId = 0b100;

	mrlock_mcas::intial(sizeof(arr) / sizeof(casRow));

	const uint32_t numThreads = 4;

	std::thread threads[numThreads];

	std::srand(std::time(nullptr));

	for (auto i = 0; i < numThreads; i++)
		threads[i] = std::thread([&] {
		for (int x=0;x<10;x++) {
			int i = rand() % 3;
			if(i==0)
				mrlock_mcas::invokeMcas(arr, &arr[2]);
			else if (i == 1)
				mrlock_mcas::invokeMcas(arr2, &arr2[2]);
			else if (i == 2)
				mrlock_mcas::invokeMcas(arr3, &arr3[2]);
		}});

	for (auto i = 0; i < numThreads; i++)
		threads[i].join();

	return 0;
}