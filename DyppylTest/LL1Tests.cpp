#include "catch2/catch.hpp"

#include "../src/Grammar.h"
#include "../src/TextStream.h"
#include "../src/tokenizer/Tokenizer.h"

#include "../src/parser/LL1.h"

#include <string_view>


struct parser_reqs {
	parser_reqs(dpl::Grammar& g) : src(""), tokenizer(src) { }

	dpl::StringStream src;
	//dpl::ParseTree tree;
	dpl::Tokenizer tokenizer;
};

using namespace std::literals::string_view_literals;
using namespace dpl::literals;

TEST_CASE("SmallGrammar", "[LL1Tests]") {
	dpl::Terminal::keywords = { {"int", 0} };
	dpl::Terminal::symbols = { {"+", 0}, {"*", 1}, {"(", 2}, {")", 3} };

	dpl::Grammar grammar{
		{ "E", {
			{ "int"_kwd },
			{ "("_sym, "E", "Op", "E", ")"_sym }
		}},
		{ "Op", {
			{ "+"_sym },
			{ "*"_sym }
		}}
	};

	parser_reqs prs{ grammar };
	dpl::LL1 parser(grammar, prs.tokenizer);

	// LL(1) parse table generation
	const auto& table = parser.generateParseTable();

	dpl::LL1::table_type expected_table{
		{ "int"_kwd, {{ "E" , 0 }}},
		{ "("_sym, {{ "E", 1 }}},
		{ "+"_sym, {{ "Op", 0 }}},
		{ "*"_sym, {{ "Op", 1 }}}
	};

	REQUIRE(table == expected_table);


	// LL(1) parsing
	prs.src.setString("(int + (int * int))");
	auto tree = parser.parse(prs.src);

	using dpl::RuleRef;

	dpl::ParseTree expected_tree{ RuleRef{grammar, "E"sv, 1}, {
		{ "("_sym },
		{ RuleRef{ grammar, "E"sv, 0 }, { "int"_kwd } },
		{ RuleRef{ grammar, "Op"sv, 0 }, {
			{ "+"_sym }
		}},
		{ RuleRef{grammar, "E"sv, 1}, {
			{ "("_sym },
			{ RuleRef{ grammar, "E"sv, 0 }, { "int"_kwd } },
			{ RuleRef{ grammar, "Op"sv, 1 }, {
				{ "*"_sym }
			}},
			{ RuleRef{ grammar, "E"sv, 0 }, { "int"_kwd } },
			{ ")"_sym }
		}},
		{ ")"_sym }
	} };

	std::cout << tree;
	std::cout << expected_tree;

	bool tree_eq = expected_tree == tree;
	REQUIRE(tree_eq);
}