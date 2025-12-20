#include <vector>

#include "Test.hpp"
#include "util/log.hpp"
#include "util/ThreadPool.hpp"

std::vector<std::string> g_filters;

int parse_args(int argc, char **argv) {
	if (argc <= 0) {
		return 0;
	}
	if (strcmp(argv[0], "-v") == 0) {
		util::g_log_flags |= util::Importance::DEBUG;
		return 1;
	} else if (strcmp(argv[0], "-vv") == 0) {
		util::g_log_flags |= util::Importance::DEBUG | util::Importance::TRACE;
		return 1;
	} else {
		g_filters.push_back(argv[0]);
		return 1;
	}
}

int main(int argc, char **argv) {
	int i = 1;
	log_info() << "flags was " << std::bitset<8>(util::g_log_flags) << std::endl;
	while (i < argc) {
		i += parse_args(argc - i, argv + i);
	}
	log_info() << "flags is " << std::bitset<8>(util::g_log_flags) << std::endl;

	auto r = test_main(g_filters);

	ThreadPool::DEFAULT.shutdown();

	return r;
}
