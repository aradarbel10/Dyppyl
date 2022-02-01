#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include "../src/Grammar.h"
#include "../src/parser/LL1.h"

#include <string_view>


using namespace std::literals::string_view_literals;
using namespace dpl::literals;

TEST_CASE("NonLL1Grammar", "[LL1Tests]") {
	std::cout << " ===== NonLL1Grammar [LL1Tests] =============================\n";

	auto [grammar, lexicon] = (
		dpl::discard |= dpl::Lexeme{ dpl::kleene{dpl::whitespace} },

		"E"nt	|= ("T"nt, ";"t)
				|  ("T"nt, "+"t, "E"nt),
		"T"nt	|= ("int"t)
				|  ("("t, "E"nt, ")"t)
	);

	bool is_ll1 = true;

	try {
		dpl::LL1 parser{ grammar, lexicon };
	} catch (...) {
		is_ll1 = false;
	}
	
	REQUIRE(!is_ll1);

}