#pragma once

#include <vector>
#include <string>
#include <optional>

namespace serial {
	struct Args {
		public:
			std::vector<std::string> source_files;
			std::string out_dir = ".";
			std::optional<std::string> debug_dir = std::nullopt;
			std::vector<std::string> unmatched;
			int verbosity_level = 0;

			int parse_args(int argc, char **argv);

			std::string arg0() const;

		private:
			int _parse_arg(int argc, char **argv);
			std::string _arg0;
	};

	extern Args g_args;
}
