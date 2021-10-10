#include "pch.h"

#include "../src/tokenizer/Token.h"

TEST(TokenTests, Terminal) {
	dpl::Terminal::keywords = { {"int", 0}, {"float", 1}, {"bool", 2} };
	dpl::Terminal::symbols = { {"+", 0}, {"-", 1}, {"*", 2}, {"/", 3} };
	

	// bimap-like behavior
	EXPECT_EQ(dpl::Terminal::keywordByIndex(0), "int");
	EXPECT_EQ(dpl::Terminal::keywordByIndex(1), "float");
	EXPECT_EQ(dpl::Terminal::keywordByIndex(2), "bool");
	EXPECT_EQ(dpl::Terminal::keywordByIndex(50), "");

	EXPECT_EQ(dpl::Terminal::symbolByIndex(1), "-");
	EXPECT_NE(dpl::Terminal::symbolByIndex(3), "*");
	EXPECT_EQ(dpl::Terminal::symbolByIndex(4), "");


	EXPECT_EQ(1, 1);
	EXPECT_TRUE(true);
}