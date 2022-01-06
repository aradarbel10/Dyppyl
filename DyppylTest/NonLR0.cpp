#include "catch2/catch.hpp"

#include "../src/Grammar.h"
#include "../src/parser/SLR.h"

#include <string_view>


using namespace std::literals::string_view_literals;
using namespace dpl::literals;

TEST_CASE("NonLR0Grammar", "[SLRTests]") {
	std::cout << " ===== NonLR0Grammar [SLRTests] =============================\n";

	auto [grammar, lexicon] = (
		dpl::discard |= dpl::Lexeme{ dpl::kleene{dpl::whitespace} },
		"E"nt	|= ("*"t, "E"nt)
				| ("*"t)
	);

	dpl::SLR parser{ grammar, lexicon };
	auto [tree, errors] = parser.parse("* * *"sv);

	std::cout << tree;
}