#include "cplusplus11.hpp"
#include <iostream>

int main(int, const char** argv)
{
	libargv thing;
	thing.init(argv);
	for (auto select : thing.argvect) {
		std::cout << '[' << static_cast<int>(select.is_flag) << ' ';
		for (auto item : select.text)
			std::cout << item << ',';
		std::cout << "]\n";
	}
	return 0;
}
