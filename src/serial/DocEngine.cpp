#include "DocEngine.hpp"

#include <filesystem>
#include <fstream>

#include "Tokenizer.hpp"
#include "codegen/ParserContext.hpp"
#include "serial/Args.hpp"
#include "util/file.hpp"
#include "util/log.hpp"

namespace serial {
	util::Result<void, Error> DocEngine::_setup_parser() {
		if (_parser) return {};

		auto context = cg::CfgContext::create(TOK_CONFIG);
		auto &c = *context;
		using T = TokenType;

		c.root("root") = c["document"] + T::Eof;

		c.prim("document") = c.cls(
			T::Whitespace
			| T::Comment
			| c["include-stmt"]
			| c["version-decl"]
		);


		c.temp("whitespace") = T::Whitespace | c.empty();

		c.prim("include-stmt") = T::Include + c["whitespace"] + T::StrConst;

		c.prim("version-frag") = T::IntConst + T::Period + T::IntConst + T::Period + T::IntConst;
		c.prim("version-decl") = T::Version + c["whitespace"] + c["version-frag"]
			+ c["whitespace"] + T::OpenCurly + c["version-blck"] + T::CloseCurly
			+ c["whitespace"];

		c.prim("version-blck") = c.cls(
			T::Whitespace
			| T::Comment
			| c["enum-decl"]
			| c["bitfield-decl"]
			| c["struct-decl"]
		);

		c.prim("enum-decl") = T::Enum + c["whitespace"] + T::Identifier
			+ c["whitespace"] + T::OpenCurly + c["enum-blck"] + T::CloseCurly;

		c.temp("enum-blck") = c.cls(T::Whitespace | T::Comment | c["enumerator"]);
		c.prim("enumerator")
			= T::Identifier + T::Semicolon
			| T::Identifier + c["whitespace"] + T::Equal + c["whitespace"] + T::IntConst + T::Semicolon;

		c.prim("bitfield-decl") = T::Bitfield + c["whitespace"] + T::Identifier
			+ c["whitespace"] + T::OpenCurly + c["bitfield-blck"] + T::CloseCurly;

		c.temp("bitfield-blck") = c.cls(T::Whitespace | T::Comment | c["bitfield-field"]);
		c.prim("bitfield-field") = T::Identifier + T::Semicolon;

		c.prim("struct-decl") = T::Struct + c["whitespace"] + T::Identifier
			+ c["whitespace"] + T::OpenCurly + c["struct-blck"] + T::CloseCurly;

		c.temp("struct-blck") = c.cls(T::Whitespace | T::Comment | c["property"]);
		c.prim("property") = c["property-type"] + c["whitespace"] + T::Identifier
			+ c["whitespace"] + T::Semicolon;

		c.temp("generics") = T::Array | T::Optional | T::UIDList;
		c.prim("property-type")
			= T::Float
			| T::Double
			| T::Boolean
			| T::U8
			| T::U16
			| T::U32
			| T::U64
			| T::I8
			| T::I16
			| T::I32
			| T::I64
			| T::Identifier
			| c["generics"] + T::Less + T::Identifier + T::Greater;

		if (auto err = c.prep().move_or()) {
			return Error(ErrorType::PARSE_ERROR, "Could not prepare parser", err.value());
		}
		c.simplify();

		if (auto err = cg::abs::AbsoluteSolver::create(std::move(context)).move_or(_parser)) {
			return Error(ErrorType::PARSE_ERROR, "Could not initialize the AbsoluteSolver", err.value());
		}

		return {};
	}

	util::Result<void, Error> DocEngine::load(
		std::vector<std::string> const &files,
		std::string const &out_dir
	) {
		if (auto err = _setup_parser().move_or()) {
			return Error(ErrorType::PARSE_ERROR, "Could not setup parser", *err);
		}

		for (auto &filename : files) {
			if (_roots.count(filename) > 0) {
				log_warning() << "File " << filename << " was already parsed." << std::endl;
			}
			auto src = util::readEnvFile(filename);
			cg::AstNode *node;
			if (auto err = _parser->parse({src.c_str(), filename.c_str()}, _parser_ctx).move_or(node)) {
				return Error(
					ErrorType::PARSE_ERROR,
					util::f("Could not parse file ", filename),
					err.value()
				);
			}
			node->compress(_parser->cfg().prim_names());

			if (g_args.debug_dir) {
				auto out_filename = std::filesystem::path(g_args.debug_dir.value());
				out_filename = out_filename / std::filesystem::path(filename + ".gv").filename();
				auto out_file = std::ofstream(out_filename);
				log_info() << "Outputting debug too " << out_filename << std::endl;
				node->print_dot(out_file, filename);
				out_file.close();
			}

			_roots[filename] = node;
		}

		return {};
	}

	cg::Parser::Ptr DocEngine::_parser;
}
