#include "pch.h"

#include "../src/tokenizer/Token.h"
#include "../src/Grammar.h"
#include "../src/parser/LL1.h"
#include "../src/TextStream.h"
#include "../src/tokenizer/Tokenizer.h"

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


TEST(GrammarTests, Production) {
	dpl::Terminal::keywords = { {"int", 0}, {"float", 1}, {"bool", 2} };
	dpl::Terminal::symbols = { {"+", 0}, {"-", 1}, {"*", 2}, {"/", 3} };

	// production construction
	dpl::ProductionRule prod_sum{ dpl::Terminal::Type::Identifier, "+"_sym, dpl::Terminal::Type::Identifier };
	dpl::ProductionRule prod_eps{};

	// epsilon productions
	EXPECT_TRUE(prod_eps.isEpsilonProd());
	EXPECT_FALSE(prod_sum.isEpsilonProd());

	// production printing
	std::ostringstream out;
	out << prod_sum;
	EXPECT_EQ(out.view(), "Identifier + Identifier");

	std::ostringstream().swap(out);
	out << prod_eps;
	EXPECT_EQ(out.view(), "epsilon");

	// rule construction
	dpl::NonterminalRules rule_exp{ "Exp", {
		{ dpl::Terminal::Type::Identifier, "-"_sym, dpl::Terminal::Type::Identifier },
		prod_sum,
		prod_eps
	}};

	// rule printing
	std::ostringstream().swap(out);
	out << rule_exp;
	EXPECT_EQ(out.view(), "    | Identifier - Identifier\n"
						  "    | Identifier + Identifier\n"
						  "    | epsilon\n");
}

//
//TEST(GrammarTests, Firsts) {
//	dpl::Terminal::keywords = { {"int", 0}, {"float", 1}, {"bool", 2} };
//	dpl::Terminal::symbols = { {"+", 0}, {"-", 1}, {"*", 2}, {"/", 3} };
//
//	dpl::Grammar non_lr0{
//		{ "E" , {
//			{ "T" },
//			{ "E", "+"_sym, "T"}
//		}},
//		{ "T", {
//			{ dpl::Terminal::Type::Number },
//			{ "("_sym, "E", ")"_sym }
//		}},
//	};
//}

struct parser_reqs {
	parser_reqs(dpl::Grammar& g) : src(""), tokenizer(src) {}

	dpl::StringStream src;
	//dpl::ParseTree tree;
	dpl::Tokenizer tokenizer;
};

TEST(LL1Tests, SmallGrammar) {
	dpl::Terminal::keywords = { {"int", 0} };
	dpl::Terminal::symbols = { {"+", 0}, {"*", 1}, {"(", 2}, {")", 3} };

	dpl::Grammar grammar{
		{ "E", {
			{ "int"_kwd },
			{ "("_sym, "E", "Op", "E", ")"_sym }
		}},
		{ "Op", {
			{ "+"_sym },
			{ "*"_sym }
		}}
	};

	parser_reqs prs{ grammar };
	dpl::LL1 parser(grammar, prs.tokenizer);

	// LL(1) parse table generation
	const auto& table = parser.generateParseTable();
	
	dpl::LL1::table_type expected_table {
		{ "int"_kwd, {{ "E" , 0 }}},
		{ "("_sym, {{ "E", 1 }}},
		{ "+"_sym, {{ "Op", 0 }}},
		{ "*"_sym, {{ "Op", 1 }}}
	};

	EXPECT_EQ(table, expected_table);


	// LL(1) parsing
	prs.src.setString("(int + (int * int))");
	while (!prs.src.closed()) {
		parser << prs.tokenizer.fetchNext();
	}
	
	dpl::ParseTree out_tree;

}

// examples to use for testing
// [l03, s164] - LL(1) parse table
// [l03, s216] - a simple LL(1) grammar
// [l03, s216] - a simple LL(1) grammar's parsing table
// [l03, s249] - epsilon-less first sets
// [l03, s282] - expanded LL(1) grammar
// [l03, s249] - first sets with epsilon
// [l03, s330] - small LL(1) parse table with epsilon
// [l4A, s030] - larger LL(1) parse table with epsilon
// [l4A, s039] - non LL(1), left-recursive grammar
// [l4A, s041] - another non LL(1) grammar
// [l4A, s062] - first and follow sets example