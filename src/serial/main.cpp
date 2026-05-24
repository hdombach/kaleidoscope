#include "DocEngine.hpp"
#include "Args.hpp"
#include "util/log.hpp"
#include "util/ThreadPool.hpp"

int main(int argc, char **argv) {
	auto r = serial::g_args.parse_args(argc, argv);
	if (r == -1) {
		log_error() << "Couldn't parse args." << std::endl;
		return 1;
	}

	auto engine = serial::DocEngine();

	if (auto err = engine.load(serial::g_args.source_files, serial::g_args.out_dir).move_or()) {
		log_error() << err.value() << std::endl;
	}

	ThreadPool::DEFAULT.shutdown();

	return 0;
}
