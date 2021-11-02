#include "catch2/catch.hpp"

#include "../src/Grammar.h"
#include "../src/TextStream.h"

#include "../src/parser/LL1.h"

#include <string_view>


using namespace std::literals::string_view_literals;
using namespace dpl::literals;

TEST_CASE("NonLL1Grammar", "[LL1Tests]") {
	std::cout << " ===== NonLL1Grammar [LL1Tests] =============================\n";

	dpl::Terminal::keywords = { {"Int", 0} };
	dpl::Terminal::symbols = { {";", 0}, {"+", 1}, {"(", 2}, {")", 3} };

	dpl::Grammar non_ll1{
		{ "E" , {
			{ "T", ";"_sym },
			{ "T", "+"_sym, "E"}
		}},
		{ "T", {
			{ "Int"_kwd },
			{ "("_sym, "E", ")"_sym }
		}}
	};

	dpl::LL1 parser{ non_ll1 };
	bool good_grammar = true;

	try {
		parser.generateParseTable();
	} catch (...) {
		good_grammar = false;
	}

	REQUIRE(!good_grammar);

}