#include "catch2/catch.hpp"

#include "../src/Grammar.h"
#include "../src/TextStream.h"

#include "../src/parser/LL1.h"

#include <string_view>


using namespace std::literals::string_view_literals;
using namespace dpl::literals;

TEST_CASE("SimpleGrammar", "[LL1Tests]") {
	std::cout << " ===== SimpleGrammar [LL1Tests] =============================\n";

	dpl::Grammar grammar {
		{ "Stmt", {
			{ "if"_kwd, "Expr", "then"_kwd, "Stmt" },
			{ "while"_kwd, "Expr", "do"_kwd, "Stmt" },
			{ "Expr", ";"_sym }
		}},
		{ "Expr", {
			{ "Term", "->"_sym, dpl::Token::Type::Identifier },
			{ "zero?"_sym, "Term" },
			{ "not"_kwd, "Expr" },
			{ "++"_sym, dpl::Token::Type::Identifier },
			{ "--"_sym, dpl::Token::Type::Identifier }
		}},
		{ "Term", {
			{ dpl::Token::Type::Identifier },
			{ dpl::Token::Type::Number }
		}}
	};
	grammar.terminal_keywords = { {"if", 0}, {"then", 1}, {"while", 2}, {"do", 3}, {"not", 4} };
	grammar.terminal_symbols = { {"->", 0}, {";", 1}, {"zero?", 2}, {"++", 3}, {"--", 4} };

	dpl::LL1 parser{ grammar };

	// LL(1) Grammar Epsilon-less First Set
	grammar.calcFirstSets();

	dpl::Grammar::firsts_type expected_firsts{
		{ "Stmt", {
			"if"_kwd, "while"_kwd, "zero?"_sym, "not"_kwd, "++"_sym, "--"_sym,
			dpl::Token::Type::Identifier, dpl::Token::Type::Number
		}},
		{ "Expr", {
			"zero?"_sym, "not"_kwd, "++"_sym, "--"_sym,
			dpl::Token::Type::Identifier, dpl::Token::Type::Number
		}},
		{ "Term", {
			dpl::Token::Type::Identifier, dpl::Token::Type::Number
		}}
	};

	REQUIRE(expected_firsts == grammar.firsts);

	// LL(1) Parse Table Generation
	const auto& table = parser.generateParseTable();

	dpl::LL1::table_type expected_table {
		{ "if"_kwd, {{ "Stmt" , 0 }}},

		{ "while"_kwd, {{ "Stmt", 1 }}},

		{ "zero?"_sym, {{ "Stmt", 2 }, { "Expr", 1 }}},
		{ "not"_kwd, {{ "Stmt", 2 }, { "Expr", 2 }}},
		{ "++"_sym, {{ "Stmt", 2 }, { "Expr", 3 }}},
		{ "--"_sym, {{ "Stmt", 2 }, { "Expr", 4 }}},

		{ dpl::Token::Type::Identifier, {{ "Stmt", 2 }, { "Expr", 0 }, { "Term", 0 }}},
		{ dpl::Token::Type::Number, {{ "Stmt", 2 }, { "Expr", 0 }, { "Term", 1 }}}
	};

	REQUIRE(table == expected_table);

	// LL(1) Parsing
	using dpl::RuleRef;

	// 1 - assignment
	auto str_src1 = dpl::StringStream{ "x -> y;" };
	auto tree1 = parser.parse(str_src1);

	dpl::ParseTree expected_tree1{ RuleRef{grammar, "Stmt", 2}, {
		{ RuleRef{ grammar, "Expr", 0 }, {
			{ RuleRef{ grammar, "Term", 0 }, {
				{ dpl::Token{ dpl::Token::Type::Identifier, "x" } }
			}},
			{ "->"_sym },
			{ dpl::Token{ dpl::Token::Type::Identifier, "y" } }
		}},
		{ ";"_sym }
	} };

	std::cout << tree1;
	std::cout << expected_tree1;

	REQUIRE(expected_tree1 == tree1);


	// 2 - while
	auto str_src2 = dpl::StringStream{ "while not zero? id\n" \
									   "	do --id;"};
	auto tree2 = parser.parse(str_src2);

	dpl::ParseTree expected_tree2{ RuleRef{grammar, "Stmt", 1}, {
		{ "while"_kwd },
		{ RuleRef{ grammar, "Expr", 2 }, {
			{ "not"_kwd },
			{ RuleRef{ grammar, "Expr", 1 }, {
				{ "zero?"_sym },
				{ RuleRef{ grammar, "Term", 0 }, {
					{ dpl::Token{ dpl::Token::Type::Identifier, "id" } }
				}}
			}}
		}},
		{ "do"_kwd },
		{ RuleRef{ grammar, "Stmt", 2 }, {
			{ RuleRef{ grammar, "Expr", 4 }, {
				{ "--"_sym },
				{ dpl::Token{ dpl::Token::Type::Identifier, "id" } }
			}},
			{ ";"_sym }
		}}
	} };

	std::cout << tree2;
	std::cout << expected_tree2;

	REQUIRE(expected_tree2 == tree2);


	// 3 - nested ifs
	auto str_src3 = dpl::StringStream{"if not zero? id then" \
									  "    if not zero? id then" \
									  "        constant -> id;"};
	auto tree3 = parser.parse(str_src3);

	dpl::ParseTree expected_tree3{ RuleRef{ grammar, "Stmt", 0 }, {
		{ "if"_kwd },
		{ RuleRef{ grammar, "Expr", 2 }, {
			{ "not"_kwd },
			{ RuleRef{ grammar, "Expr", 1 }, {
				{ "zero?"_sym },
				{ RuleRef{ grammar, "Term", 0 }, {
					{ dpl::Token{ dpl::Token::Type::Identifier, "id" } }
				}}
			}}
		}},
		{ "then"_kwd },
		{ RuleRef{ grammar, "Stmt", 0 }, {
			{ "if"_kwd },
			{ RuleRef{ grammar, "Expr", 2 }, {
				{ "not"_kwd },
				{ RuleRef{ grammar, "Expr", 1 }, {
					{ "zero?"_sym },
					{ RuleRef{ grammar, "Term", 0 }, {
						{ dpl::Token{ dpl::Token::Type::Identifier, "id" } }
					}}
				}}
			}},
			{ "then"_kwd },
			{ RuleRef{grammar, "Stmt", 2}, {
				{ RuleRef{ grammar, "Expr", 0 }, {
					{ RuleRef{ grammar, "Term", 0 }, {
						{ dpl::Token{ dpl::Token::Type::Identifier, "constant" } }
					}},
					{ "->"_sym },
					{ dpl::Token{ dpl::Token::Type::Identifier, "id" } }
				}},
				{ ";"_sym }
			}}
		}}
	} };

	std::cout << tree3;
	std::cout << expected_tree3;

	REQUIRE(expected_tree3 == tree3);
}