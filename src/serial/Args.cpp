#include "Args.hpp"
#include "util/log.hpp"

#include <vector>
#include <string>

namespace serial {
	int Args::parse_args(int argc, char **argv) {
		_arg0 = argv[0];
		int i = 1;
		while (i < argc) {
			auto r = _parse_arg(argc - i, argv + i);
			if (r == -1) return r;
			i += r;
		}

		return i;
	}

	int Args::_parse_arg(int argc, char **argv) {
		if (strcmp(argv[0], "-o") == 0) {
			out_dir = argv[1];
			return 2;
		} else if (strcmp(argv[0], "-d") == 0) {
			debug_dir = argv[1];
			return 2;
		} else if (strcmp(argv[0], "-v") == 0) {
			g_args.verbosity_level++;
			util::g_log_flags |= Importance::INFO;
			return 1;
		} else {
			if (argv[0][0] == '-') {
				log_error() << "Unknown arg: " << argv[0] << std::endl;
				return -1;
			}
			source_files.push_back(argv[0]);
			return 1;
		}
	}

	Args g_args;
}
