#include <iostream>

#include "mcshub_arguments.hpp"
#include "mcshub.hpp"

int main(int argc, char *argv[]) {
	if (argc == 0) {
		std::cerr << "your system acts strange" << std::endl;
		return EXIT_FAILURE;
	}
	try {
		mcshub::arguments.parse(argc, argv);
	} catch (const std::exception & e) {
		std::cerr << "invalid program arguments: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	try {
		return mcshub::entrypoint(argv[0]);
	} catch (const std::exception & e) {
		std::cerr << "fatal error: " << typeid(e).name() << ": " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}
