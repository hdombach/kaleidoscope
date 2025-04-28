#include "PrintTools.hpp"
#include "RectSize.hpp"
#include "util/lines_iterator.hpp"
#include "util/log.hpp"

#include <iomanip>
#include <ostream>

namespace util {
	/**
	 * @brief Prints a row in a table given a palet
	 * @param[in] os ostream to print to
	 * @param[in] widths The widths of the cells to draw
	 * @param[in] palet The characters to use when printing
	 *
	 * Need to have a vector of const char* to properly print unicode characters
	 */
	void _print_row(
		std::ostream &os,
		std::vector<size_t> const &widths,
		std::vector<const char *> const &palet
	) {
		for (uint32_t x = 0; x < widths.size(); x++) {
			if (x == 0) {
				os << palet[0];
			} else {
				os << palet[1];
			}
			for (int i = 0; i < widths[x]; i++) os << palet[3];
		}
		os << palet[2] << std::endl;
	}

	/**
	 * @brief Prints a row with content in a table given a palet
	 * @param[in] os ostream to print to
	 * @param[in] widths The widths of the cells to draw
	 * @param[in] palet The characters to use when printing
	 * @param[in] content Items to print
	 *
	 * Need to have a vector of const char* to properly print unicode characters
	 */
	void _print_row_content(
		std::ostream &os,
		std::vector<size_t> const &widths,
		std::vector<const char *> const &palet,
		std::vector<std::string_view> const &content
	) {
		for (uint32_t x = 0; x < widths.size(); x++) {
			os << palet[0];
			os << std::left << std::setw(widths[x]) << content[x];
		}
		os << palet[0] << std::endl;
	}


	void print_table(
		std::ostream &os,
		const std::vector<std::vector<std::string>> &table
	) {
		auto widths = std::vector<size_t>(table[0].size(), 0);
		auto heights = std::vector<size_t>(table.size(), 0);

		uint32_t y = 0, x = 0;
		for (auto &row : table) {
			x = 0;
			for (auto &cell : row) {
				auto size = str_rect(table[y][x]);
				log_debug() << "size is " << size.w << "x" << size.h << std::endl;

				if (size.w > widths[x]) {
					widths[x] = size.w;
				}

				if (size.h > heights[y]) {
					heights[y] = size.h;
				}

				x++;
			}
			y++;
		}


		for (y = 0; y < heights.size(); y++) {
			if (y == 0) {
				_print_row(os, widths, {"┌", "┬", "┐", "─"});
			} else {
				_print_row(os, widths, {"├", "┼", "┤", "─"});
			}
			for (int y2 = 0; y2 < heights[y]; y2++) {
				auto row = std::vector<std::string_view>();
				for (auto x = 0; x < widths.size(); x++) {
					auto s = util::lines_iterator::begin(table[y][x])[y2];
					row.push_back(s);
				}
				_print_row_content(os, widths, {"│"}, row);
			}
		}
		_print_row(os, widths, {"└", "┴", "┘", "─"});

	}

	RectSize str_rect(const std::string &str) {
		auto rect = RectSize(0, 0);
		for (auto line : util::get_lines(str)) {
			if (line.size() > rect.w) {
				rect.w = line.size();
			}
			rect.h++;
		}
		return rect;
	}
}
