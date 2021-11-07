#include "catch2/catch.hpp"

#include "../src/tokenizer/Token.h"

using namespace dpl::literals;

TEST_CASE("Terminal", "[TokenTests]") {
	std::cout << " ===== Terminal [TokenTests] =============================\n";

	// comparisons
	constexpr dpl::Terminal unk{ dpl::Terminal::Type::Unknown };
	constexpr dpl::Terminal iden{ dpl::Terminal::Type::Identifier };
	constexpr dpl::Terminal sym{ dpl::Terminal::Type::Symbol, "-" };

	REQUIRE(sym != iden);
	static_assert(sym != iden);

	REQUIRE(iden == iden);
	REQUIRE(sym == sym);
	REQUIRE(unk == unk);
	REQUIRE(unk == sym);
	REQUIRE(unk == iden);

	static_assert(iden == iden);
	static_assert(sym == sym);
	static_assert(unk == unk);
	static_assert(unk == sym);
	static_assert(unk == iden);

	// terminal stringification
	dpl::Terminal eof{ dpl::Terminal::Type::EndOfFile };
	dpl::Terminal kwd{ dpl::Terminal::Type::Keyword, "bool" };

	REQUIRE(unk.stringify() == "Unknown");
	REQUIRE(iden.stringify() == "Identifier");
	REQUIRE(sym.stringify() == "-");
	REQUIRE(eof.stringify() == "EndOfFile");
	REQUIRE(kwd.stringify() == "bool");


	// terminal literals
	REQUIRE(kwd == "bool"_kwd);
	REQUIRE(sym == "-"_sym);
}


TEST_CASE("Token", "[TokenTests]") {
	// comparisons
	dpl::Token unk{ dpl::Token::Type::Unknown };
	dpl::Token str{ dpl::Token::Type::String, "heyo" };
	dpl::Token num1{ dpl::Token::Type::Number, 3.1415 };
	dpl::Token num2{ dpl::Token::Type::Number, 2.7182 };

	REQUIRE(unk == unk);
	REQUIRE(unk == str);
	REQUIRE(unk == num1);
	REQUIRE(unk == num2);

	REQUIRE(str != num1);
	REQUIRE(num1 != num2);

	REQUIRE(str == str);
	REQUIRE(num2 == num2);

	// token stringification
	dpl::Token iden{ dpl::Token::Type::Identifier, "var" };
	dpl::Token eof{ dpl::Token::Type::EndOfFile };
	dpl::Token kwd{ "int"_kwd };
	dpl::Token sym{ "*"_sym };

	REQUIRE(unk.stringify() == "[Unknown]");
	REQUIRE(str.stringify() == "[\"heyo\", String]");
	REQUIRE(num1.stringify() == "[3.141500, Number]");
	REQUIRE(iden.stringify() == "[var, Identifier]");
	REQUIRE(eof.stringify() == "[EndOfFile]");
	REQUIRE(kwd.stringify() == "[int, Keyword]");
	REQUIRE(sym.stringify() == "[*, Symbol]");

}