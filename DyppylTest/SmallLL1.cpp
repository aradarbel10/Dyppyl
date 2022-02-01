#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include "../src/Grammar.h"
#include "../src/parser/LL1.h"

#include <string_view>


using namespace std::literals::string_view_literals;
using namespace dpl::literals;

TEST_CASE("SmallGrammar", "[LL1Tests]") {
	std::cout << " ===== SmallGrammar [LL1Tests] =============================\n";

	auto [grammar, lexicon] = (
		dpl::discard	|= dpl::Lexeme{ dpl::kleene{dpl::whitespace} },
		"E"nt			|= ("int"t)
						|  ("("t, "E"nt, "Op"nt, "E"nt, ")"t),
		"Op"nt			|= "+"t | "*"t
	);

	dpl::LL1 parser{ grammar, lexicon };

	// LL(1) Parse Table Generation
	const auto& table = parser.getParseTable();

	auto expected_table = dpl::LLTable{grammar, {
		{ "int"t, {{ "E" , 0 }}},
		{ "("t, {{ "E", 1 }}},
		{ "+"t, {{ "Op", 0 }}},
		{ "*"t, {{ "Op", 1 }}}
	}};

	REQUIRE(table.compareTo(expected_table));


	// LL(1) Parsing
	auto [tree, errors] = parser.parse("(int + (int * int))");

	using dpl::RuleRef;

	dpl::ParseTree<> expected_tree{ RuleRef{"E", 1}, {
		{ "("tkn },
		{ RuleRef{ "E", 0 }, {{ "int"tkn }}},
		{ RuleRef{ "Op", 0 }, {
			{ "+"tkn }
		}},
		{ RuleRef{"E", 1}, {
			{ "("tkn },
			{ RuleRef{ "E", 0 }, {{ "int"tkn }} },
			{ RuleRef{ "Op", 1 }, {
				{ "*"tkn }
			}},
			{ RuleRef{ "E", 0 }, {{ "int"tkn }} },
			{ ")"tkn }
		}},
		{ ")"tkn }
	} };

	std::cout << tree;
	std::cout << expected_tree;

	REQUIRE(expected_tree == tree);
}