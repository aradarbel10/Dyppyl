#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include "../src/Grammar.h"
#include "../src/parser/LL1.h"
#include "../src/TextStream.h"
#include "../src/tokenizer/Tokenizer.h"


TEST_CASE("Production", "[GrammarTests]") {
	std::cout << " ===== Production [GrammarTests] =============================\n";

	using namespace std::literals::string_view_literals;
	using namespace dpl::literals;

	// production construction
	dpl::ProductionRule prod_sum{ dpl::Terminal::Type::Identifier, "+"_sym, dpl::Terminal::Type::Identifier };
	dpl::ProductionRule prod_eps{};

	// epsilon productions
	REQUIRE( prod_eps.isEpsilonProd());
	REQUIRE(!prod_sum.isEpsilonProd());

	// production printing
	std::ostringstream out;
	out << prod_sum;
	REQUIRE(out.view() == "Identifier + Identifier");

	std::ostringstream().swap(out);
	out << prod_eps;
	REQUIRE(out.view() == "epsilon");

	// rule construction
	dpl::NonterminalRules rule_exp{ "Exp", {
		{ dpl::Terminal::Type::Identifier, "-"_sym, dpl::Terminal::Type::Identifier },
		prod_sum,
		prod_eps
	}};

	// rule printing
	std::ostringstream().swap(out);
	out << rule_exp;
	REQUIRE(out.view() == "    | Identifier - Identifier\n"
						  "    | Identifier + Identifier\n"
						  "    | epsilon\n");
}


// examples to use for testing
// [l03, s164] - LL(1) parse table V
// [l03, s216] - a simple LL(1) grammar V
// [l03, s216] - a simple LL(1) grammar's parsing table V
// [l03, s249] - epsilon-less first sets V
// [l03, s282] - expanded LL(1) grammar
// [l03, s249] - first sets with epsilon
// [l03, s330] - small LL(1) parse table with epsilon
// [l4A, s030] - larger LL(1) parse table with epsilon
// [l4A, s039] - non LL(1), left-recursive grammar
// [l4A, s041] - another non LL(1) grammar
// [l4A, s062] - first and follow sets example