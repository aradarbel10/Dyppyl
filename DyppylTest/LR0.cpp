#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include "../src/Grammar.h"
#include "../src/parser/LR.h"

#include <string_view>


TEST_CASE("SimpleMathGrammar", "[LR0Tests]") {
	std::cout << " ===== SimpleGrammar [LR0Tests] =============================\n";
	
	using namespace std::literals::string_view_literals;
	using namespace dpl::literals;

	auto [grammar, lexicon] = (
		dpl::discard	|= dpl::Lexeme{ dpl::kleene{dpl::whitespace} },
		"num"t			|= dpl::Lexeme{ dpl::some{dpl::digit}, [](std::string_view str) -> long double { return dpl::from_string<int>(str); } },

		"E"nt			|= ("T"nt, ";"t)
						|  ("T"nt, "+"t, "E"nt),
		"T"nt			|= ("num"t)
						|  ("("t, "E"nt, ")"t)
	);

	dpl::LR0 parser{ grammar, lexicon };

	auto [tree, errors] = parser.parse("5 + ( 10 + 3; );");


	using dpl::RuleRef;

	dpl::ParseTree<> expected_tree{ RuleRef{ "E", 1 }, {
		{ RuleRef{ "T", 0 }, {
			{ dpl::Token<>{ "num"sv, 5.0 } }
		}},
		{ "+"tkn },
		{ RuleRef{ "E", 0 }, {
			{ RuleRef{ "T", 1 }, {
				{ "("tkn },
				{ RuleRef{ "E", 1 }, {
					{ RuleRef{ "T", 0 }, {
						{ dpl::Token<>{ "num"sv, 10.0 } }
					}},
					{ "+"tkn },
					{ RuleRef{ "E", 0 }, {
						{ RuleRef{ "T", 0 }, {
							{ dpl::Token<>{ "num"sv, 3.0 } }
						}},
						{ ";"tkn }
					}}
				}},
				{ ")"tkn }
			}},
			{ ";"tkn }
		}}
	}};

	std::cout << tree;
	std::cout << expected_tree;

	REQUIRE(tree == expected_tree);
}


// examples to use for testing
// [l04b, s66] - lR0 example 1
// [l05, s171] - LR0 example 2
// [l05, s255] - LR0 parse table 1
// [l05, s383] - LR1 parse table 2
// [l06,  s93] - SLR1 example