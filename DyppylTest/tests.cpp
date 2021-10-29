#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include "../src/tokenizer/Token.h"
#include "../src/Grammar.h"
#include "../src/parser/LL1.h"
#include "../src/TextStream.h"
#include "../src/tokenizer/Tokenizer.h"

TEST_CASE("Terminal", "[TokenTests]") {
	dpl::Terminal::keywords = { {"int", 0}, {"float", 1}, {"bool", 2} };
	dpl::Terminal::symbols = { {"+", 0}, {"-", 1}, {"*", 2}, {"/", 3} };
	

	// bimap-like behavior
	REQUIRE(dpl::Terminal::keywordByIndex(0)  == "int");
	REQUIRE(dpl::Terminal::keywordByIndex(1)  == "float");
	REQUIRE(dpl::Terminal::keywordByIndex(2)  == "bool");
	REQUIRE(dpl::Terminal::keywordByIndex(50) == "");

	REQUIRE(dpl::Terminal::symbolByIndex(1) == "-");
	REQUIRE(dpl::Terminal::symbolByIndex(3) != "*");
	REQUIRE(dpl::Terminal::symbolByIndex(4) == "");

	// comparisons
	dpl::Terminal unk{ dpl::Terminal::Type::Unknown };
	dpl::Terminal iden{ dpl::Terminal::Type::Identifier };
	dpl::Terminal sym{ dpl::Terminal::Type::Symbol, size_t{1} };

	REQUIRE(sym != iden);

	REQUIRE(iden == iden);
	REQUIRE(sym  == sym);
	REQUIRE(unk  == unk);
	REQUIRE(unk  == sym);
	REQUIRE(unk  == iden);

	// terminal stringification
	dpl::Terminal eof{ dpl::Terminal::Type::EndOfFile };
	dpl::Terminal kwd{ dpl::Terminal::Type::Keyword, size_t{2} };

	REQUIRE(unk.stringify()  == "Unknown");
	REQUIRE(iden.stringify() == "Identifier");
	REQUIRE(sym.stringify()  == "-");
	REQUIRE(eof.stringify()  == "EndOfFile");
	REQUIRE(kwd.stringify()  == "bool");


	// terminal literals
	REQUIRE(kwd == "bool"_kwd);
	REQUIRE(sym == "-"_sym);
}


TEST_CASE("Token", "[TokenTests]") {
	dpl::Terminal::keywords = { {"int", 0}, {"float", 1}, {"bool", 2} };
	dpl::Terminal::symbols = { {"+", 0}, {"-", 1}, {"*", 2}, {"/", 3} };


	// comparisons
	dpl::Token unk{ dpl::Token::Type::Unknown };
	dpl::Token str{ dpl::Token::Type::String, "heyo" };
	dpl::Token num1{ dpl::Token::Type::Number, 3.1415 };
	dpl::Token num2{ dpl::Token::Type::Number, 2.7182 };

	REQUIRE(unk == unk);
	REQUIRE(unk == str);
	REQUIRE(unk == num1);
	REQUIRE(unk == num2);

	REQUIRE(str  != num1);
	REQUIRE(num1 != num2);

	REQUIRE(str  == str);
	REQUIRE(num2 == num2);

	// token stringification
	dpl::Token iden{ dpl::Token::Type::Identifier, "var"};
	dpl::Token eof{ dpl::Token::Type::EndOfFile };
	dpl::Token kwd{ dpl::Token::Type::Keyword, size_t{0} };
	dpl::Token sym{ dpl::Token::Type::Symbol, size_t{2} };

	REQUIRE(unk.stringify()  == "[Unknown]");
	REQUIRE(str.stringify()  == "[\"heyo\", String]");
	REQUIRE(num1.stringify() == "[3.141500, Number]");
	REQUIRE(iden.stringify() == "[var, Identifier]");
	REQUIRE(eof.stringify()  == "[EndOfFile]");
	REQUIRE(kwd.stringify()  == "[int, Keyword]");
	REQUIRE(sym.stringify()  == "[*, Symbol]");

}


TEST_CASE("Production", "[GrammarTests]") {
	dpl::Terminal::keywords = { {"int", 0}, {"float", 1}, {"bool", 2} };
	dpl::Terminal::symbols = { {"+", 0}, {"-", 1}, {"*", 2}, {"/", 3} };

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

TEST_CASE("SmallGrammar", "[LL1Tests]") {
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

	REQUIRE(table == expected_table);


	// LL(1) parsing
	prs.src.setString("(int + (int * int))");
	while (!prs.src.closed()) {
		parser << prs.tokenizer.fetchNext();
	}
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