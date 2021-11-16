#include "catch2/catch.hpp"

#include "../src/Grammar.h"
#include "../src/TextStream.h"

#include "../src/parser/LR.h"

#include <string_view>


using namespace std::literals::string_view_literals;
using namespace dpl::literals;

TEST_CASE("SimpleMathGrammar", "[LR0Tests]") {
	std::cout << " ===== SimpleGrammar [LR0Tests] =============================\n";

	dpl::Grammar grammar{
		{ "S" , {
			{ "E" },
		}},
		{ "E" , {
			{ "T", ";"_sym },
			{ "T", "+"_sym, "E"}
		}},
		{ "T", {
			{ dpl::Terminal::Type::Number },
			{ "("_sym, "E", ")"_sym }
		}}
	};

	grammar.symbols = { "+", ";", "(", ")" };

	dpl::LR0 parser{ grammar };

	auto src = dpl::StringStream("5 + ( 10 + 1.5; );");
	auto tree = parser.parse(src);


	using dpl::RuleRef;

	dpl::ParseTree expected_tree{ RuleRef{ grammar, "S", 0 }, {
		{ RuleRef{ grammar, "E", 1 }, {
			{ RuleRef{ grammar, "T", 0 }, {
				{dpl::Token{ dpl::Token::Type::Number, 5.0 }}
			}},
			{ "+"_sym },
			{ RuleRef{ grammar, "E", 0 }, {
				{ RuleRef{ grammar, "T", 1 }, {
					{ "("_sym },
					{ RuleRef{ grammar, "E", 1 }, {
						{ RuleRef{ grammar, "T", 0 }, {
							{dpl::Token{ dpl::Token::Type::Number, 10.0 }}
						}},
						{ "+"_sym },
						{ RuleRef{ grammar, "E", 0 }, {
							{ RuleRef{ grammar, "T", 0 }, {
								{dpl::Token{ dpl::Token::Type::Number, 1.5 }}
							}},
							{ ";"_sym }
						}}
					}},
					{ ")"_sym }
				}},
				{ ";"_sym }
			}}
		}}
	} };

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