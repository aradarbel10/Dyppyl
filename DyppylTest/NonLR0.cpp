#include "catch2/catch.hpp"

#include "../src/Grammar.h"
#include "../src/parser/SLR.h"

#include <string_view>


using namespace std::literals::string_view_literals;
using namespace dpl::literals;

TEST_CASE("NonLR0Grammar", "[SLRTests]") {
	std::cout << " ===== NonLR0Grammar [SLRTests] =============================\n";

	dpl::Grammar non_lr0{
		"E"nt	|= ("*"t, "E"nt)
				|  ("*"t)
	};

	dpl::SLR parser{ non_lr0 };
	auto [tree, errors] = parser.parse("* * *"sv);

	std::cout << tree;
}