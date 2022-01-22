#define CATCH_CONFIG_MAIN
#include "../deps/include/catch2/catch.hpp"

#include "../src/Grammar.h"
#include "../src/parser/LL1.h"

#include <string_view>

using namespace std::literals::string_view_literals;
using namespace dpl::literals;

TEST_CASE("TopDownBuilder", "[LL1Tests]") {
    auto [grammar, lexicon] = (
		dpl::discard	|= dpl::Lexeme{ dpl::kleene{dpl::whitespace} },
		"E"nt			|= (!"int"t)
						|  (!"func"t, ~"("t, *"Es"nt, ~")"t)
						|  (~"("t, "E"nt, !"Op"nt, "E"nt, ~")"t),
		"Es"nt			|= ("E"nt, *"Es"nt) | dpl::epsilon,
		"Op"nt			|= !"+"t | !"*"t
	);

    dpl::LL1 parser{ grammar, lexicon };
	auto [tree, errors] = parser.parse("(func((int+(int*int)) int func((int * int))) + int)");

    using dpl::RuleRef;

	dpl::ParseTree<> expected_tree{ "+"tkn, {
		{ "func"tkn, {
			{ "+"tkn, {
                { "int"tkn },
                { "*"tkn, {
                    { "int"tkn },
                    { "int"tkn }
                }}
            }},
            { "int"tkn },
            { "func"tkn, {
                { "*"tkn, {
                    { "int"tkn },
                    { "int"tkn }
                }}
            }}
		}},
        { "int"tkn }
	}};

    REQUIRE(expected_tree == tree);
}