#include <iostream>
#include "seq_mcas.h"

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

	arr[1] = {};
	arr[1].address = &two;
	arr[1].expectedValue = 2;
	arr[1].newValue = 38;

	arr[2] = {};
	arr[2].address = &three;
	arr[2].expectedValue = 3;
	arr[2].newValue = 39;


	std::cout << "Before: " << one << ", " << two << ", " << three << std::endl;
	seq_mcas::invokeMcas(arr, &arr[2]);
	std::cout << "After: " << one << ", " << two << ", " << three << std::endl;

	return 0;
}