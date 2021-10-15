#include "pch.h"

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

	// comparisons
	dpl::Terminal unk{ dpl::Terminal::Type::Unknown };
	dpl::Terminal iden{ dpl::Terminal::Type::Identifier };
	dpl::Terminal sym{ dpl::Terminal::Type::Symbol, size_t{1} };

	EXPECT_NE(sym, iden);

	EXPECT_EQ(iden, iden);
	EXPECT_EQ(sym, sym);
	EXPECT_EQ(unk, unk);
	EXPECT_EQ(unk, sym);
	EXPECT_EQ(unk, iden);

	// terminal stringification
	dpl::Terminal eof{ dpl::Terminal::Type::EndOfFile };
	dpl::Terminal kwd{ dpl::Terminal::Type::Keyword, size_t{2} };

	EXPECT_EQ(unk.stringify(), "Unknown");
	EXPECT_EQ(iden.stringify(), "Identifier");
	EXPECT_EQ(sym.stringify(), "-");
	EXPECT_EQ(eof.stringify(), "EndOfFile");
	EXPECT_EQ(kwd.stringify(), "bool");


	// terminal literals
	EXPECT_EQ(kwd, "bool"_kwd);
	EXPECT_EQ(sym, "-"_sym);
}


TEST(TokenTests, Token) {
	dpl::Terminal::keywords = { {"int", 0}, {"float", 1}, {"bool", 2} };
	dpl::Terminal::symbols = { {"+", 0}, {"-", 1}, {"*", 2}, {"/", 3} };


	// comparisons
	dpl::Token unk{ dpl::Token::Type::Unknown };
	dpl::Token str{ dpl::Token::Type::String, "heyo" };
	dpl::Token num1{ dpl::Token::Type::Number, 3.1415 };
	dpl::Token num2{ dpl::Token::Type::Number, 2.7182 };

	EXPECT_EQ(unk, unk);
	EXPECT_EQ(unk, str);
	EXPECT_EQ(unk, num1);
	EXPECT_EQ(unk, num2);

	EXPECT_NE(str, num1);
	EXPECT_NE(num1, num2);

	EXPECT_EQ(str, str);
	EXPECT_EQ(num2, num2);

	// token stringification
	dpl::Token iden{ dpl::Token::Type::Identifier, "var"};
	dpl::Token eof{ dpl::Token::Type::EndOfFile };
	dpl::Token kwd{ dpl::Token::Type::Keyword, size_t{0} };
	dpl::Token sym{ dpl::Token::Type::Symbol, size_t{2} };

	EXPECT_EQ(unk.stringify(), "[Unknown]");
	EXPECT_EQ(str.stringify(), "[\"heyo\", String]");
	EXPECT_EQ(num1.stringify(),"[3.141500, Number]");
	EXPECT_EQ(iden.stringify(),"[var, Identifier]");
	EXPECT_EQ(eof.stringify(), "[EndOfFile]");
	EXPECT_EQ(kwd.stringify(), "[int, Keyword]");
	EXPECT_EQ(sym.stringify(), "[*, Symbol]");

}