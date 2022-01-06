#include "catch2/catch.hpp"

#include "../src/Grammar.h"
#include "../src/TextStream.h"

#include "../src/parser/LL1.h"

#include <string_view>


using namespace std::literals::string_view_literals;
using namespace dpl::literals;

TEST_CASE("SimpleGrammar", "[LL1Tests]") {
	std::cout << " ===== SimpleGrammar [LL1Tests] =============================\n";

	auto [grammar, lexicon] = (
		dpl::discard	|= dpl::Lexeme{ dpl::kleene{dpl::whitespace} },
		"num"t			|= dpl::Lexeme{ dpl::some{dpl::digit}, [](std::string_view str) -> long double { return dpl::from_string<int>(str); } },
		"id"t			|= dpl::Lexeme{ dpl::some{dpl::alpha} },

		"Stmt"nt		|= ("if"t, "Expr"nt, "then"t, "Stmt"nt)
						|  ("while"t, "Expr"nt, "do"t, "Stmt"nt)
						|  ("Expr"nt, ";"t),
		"Expr"nt		|= ("Term"nt, "->"t, "id"t)
						|  ("zero?"t, "Term"nt)
						|  ("not"t, "Expr"nt)
						|  ("++"t, "id"t)
						|  ("--"t, "id"t),
		"Term"nt		|= "id"t | "num"t
	);

	dpl::LL1 parser{ grammar, lexicon };

	// LL(1) Grammar Epsilon-less First Set
	grammar.initialize();

	dpl::Grammar<>::firsts_type expected_firsts{
		{ "Stmt", {
			"if"t, "while"t, "zero?"t, "not"t, "++"t, "--"t, "id"t, "num"t
		}},
		{ "Expr", {
			"zero?"t, "not"t, "++"t, "--"t, "id"t, "num"t
		}},
		{ "Term", {
			"id"t, "num"t
		}}
	};

	REQUIRE(expected_firsts == grammar.get_firsts());

	// LL(1) Parse Table Generation
	const auto& table = parser.getParseTable();

	auto expected_table = dpl::LLTable{grammar, {
		{ "if"t, {{ "Stmt" , 0 }}},

		{ "while"t, {{ "Stmt", 1 }}},

		{ "zero?"t, {{ "Stmt", 2 }, { "Expr", 1 }}},
		{ "not"t, {{ "Stmt", 2 }, { "Expr", 2 }}},
		{ "++"t, {{ "Stmt", 2 }, { "Expr", 3 }}},
		{ "--"t, {{ "Stmt", 2 }, { "Expr", 4 }}},

		{ "id"t, {{"Stmt", 2}, {"Expr", 0}, {"Term", 0}}},
		{ "num"t, {{"Stmt", 2}, {"Expr", 0}, {"Term", 1}}}
	}};

	REQUIRE(table.compareTo(expected_table));

	// LL(1) Parsing
	using dpl::RuleRef;

	// 1 - assignment
	auto [tree1, errors1] = parser.parse("x -> y;");

	dpl::ParseTree<> expected_tree1{ RuleRef{ "Stmt", 2 }, {
		{ RuleRef{ "Expr", 0 }, {
			{ RuleRef{ "Term", 0 }, {
				{ dpl::Token<>{ "id"sv, "x"t } }
			}},
			{ "->"tkn },
			{ dpl::Token<>{ "id"sv, "y"t } }
		}},
		{ ";"tkn }
	} };

	std::cout << tree1;
	std::cout << expected_tree1;

	REQUIRE(expected_tree1 == tree1);


	// 2 - while
	auto [tree2, errors2] = parser.parse("while not zero? id\n"
										 "	do --id;");

	dpl::ParseTree<> expected_tree2{ RuleRef{ "Stmt", 1 }, {
		{ "while"tkn },
		{ RuleRef{ "Expr", 2 }, {
			{ "not"tkn },
			{ RuleRef{ "Expr", 1 }, {
				{ "zero?"tkn },
				{ RuleRef{ "Term", 0 }, {
					{ dpl::Token<>{ "id"sv, "id"t } }
				}}
			}}
		}},
		{ "do"tkn },
		{ RuleRef{ "Stmt", 2 }, {
			{ RuleRef{ "Expr", 4 }, {
				{ "--"tkn },
				{ dpl::Token<>{ "id"sv, "id"t } }
			}},
			{ ";"tkn }
		}}
	} };

	std::cout << tree2;
	std::cout << expected_tree2;

	REQUIRE(expected_tree2 == tree2);


	// 3 - nested ifs
	auto [tree3, errors3] = parser.parse(
		"if not zero? id then"
		"    if not zero? id then"
		"        constant -> id;"
	);

	dpl::ParseTree<> expected_tree3{ RuleRef{ "Stmt", 0 }, {
		{ "if"tkn },
		{ RuleRef{ "Expr", 2 }, {
			{ "not"tkn },
			{ RuleRef{ "Expr", 1 }, {
				{ "zero?"tkn },
				{ RuleRef{ "Term", 0 }, {
					{ dpl::Token<>{ "id"sv, "id"t } }
				}}
			}}
		}},
		{ "then"tkn },
		{ RuleRef{ "Stmt", 0 }, {
			{ "if"tkn },
			{ RuleRef{ "Expr", 2 }, {
				{ "not"tkn },
				{ RuleRef{ "Expr", 1 }, {
					{ "zero?"tkn },
					{ RuleRef{ "Term", 0 }, {
						{ dpl::Token<>{ "id"sv, "id"t } }
					}}
				}}
			}},
			{ "then"tkn },
			{ RuleRef{ "Stmt", 2}, {
				{ RuleRef{ "Expr", 0 }, {
					{ RuleRef{ "Term", 0 }, {
						{ dpl::Token<>{ "id"sv, "constant"t }} 
					}},
					{ "->"tkn },
					{ dpl::Token<>{ "id"sv, "id"t } }
				}},
				{ ";"tkn }
			}}
		}}
	} };

	std::cout << tree3;
	std::cout << expected_tree3;

	REQUIRE(expected_tree3 == tree3);
}