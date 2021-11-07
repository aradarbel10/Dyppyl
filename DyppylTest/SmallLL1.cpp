#include "catch2/catch.hpp"

#include "../src/Grammar.h"
#include "../src/TextStream.h"

#include "../src/parser/LL1.h"

#include <string_view>


using namespace std::literals::string_view_literals;
using namespace dpl::literals;

TEST_CASE("SmallGrammar", "[LL1Tests]") {
	std::cout << " ===== SmallGrammar [LL1Tests] =============================\n";

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
	grammar.terminal_keywords = { {"int", 0} };
	grammar.terminal_symbols = { {"+", 0}, {"*", 1}, {"(", 2}, {")", 3} };

	dpl::LL1 parser{ grammar };

	// LL(1) Parse Table Generation
	const auto& table = parser.generateParseTable();

	dpl::LL1::table_type expected_table{
		{ "int"_kwd, {{ "E" , 0 }}},
		{ "("_sym, {{ "E", 1 }}},
		{ "+"_sym, {{ "Op", 0 }}},
		{ "*"_sym, {{ "Op", 1 }}}
	};

	REQUIRE(table == expected_table);


	// LL(1) Parsing
	auto str_src = dpl::StringStream{ "(int + (int * int))" };
	auto tree = parser.parse(str_src);

	using dpl::RuleRef;

	dpl::ParseTree expected_tree{ RuleRef{grammar, "E", 1}, {
		{ "("_sym },
		{ RuleRef{ grammar, "E", 0 }, { "int"_kwd } },
		{ RuleRef{ grammar, "Op", 0 }, {
			{ "+"_sym }
		}},
		{ RuleRef{grammar, "E", 1}, {
			{ "("_sym },
			{ RuleRef{ grammar, "E", 0 }, { "int"_kwd } },
			{ RuleRef{ grammar, "Op", 1 }, {
				{ "*"_sym }
			}},
			{ RuleRef{ grammar, "E", 0 }, { "int"_kwd } },
			{ ")"_sym }
		}},
		{ ")"_sym }
	} };

	std::cout << tree;
	std::cout << expected_tree;

	REQUIRE(expected_tree == tree);
}