#include "pch.h"

TEST(GrammarTests, Production) {
	dpl::Terminal::keywords = { {"int", 0}, {"float", 1}, {"bool", 2} };
	dpl::Terminal::symbols = { {"+", 0}, {"-", 1}, {"*", 2}, {"/", 3} };

	// production construction
	dpl::ProductionRule prod_sum{ dpl::Terminal::Type::Identifier, "+"_sym, dpl::Terminal::Type::Identifier };
	dpl::ProductionRule prod_eps{};

	// epsilon productions
	EXPECT_TRUE( prod_eps.isEpsilonProd());
	EXPECT_FALSE(prod_sum.isEpsilonProd());

	// production printing

	// rule construction
	dpl::NonterminalRules rule_exp{ "Exp", {
		{ dpl::Terminal::Type::Identifier, "-"_sym, dpl::Terminal::Type::Identifier },
		prod_sum,
		prod_eps
	}};

	// rule printing
}