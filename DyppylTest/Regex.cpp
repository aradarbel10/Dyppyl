#include "catch2/catch.hpp"

#include "../src/Regex.h"

TEST_CASE("exact", "[RegexTests] [match]") {
	constexpr std::string_view text = "abc";
	constexpr auto lex = dpl::match{ "abc" };

	REQUIRE(lex(text.begin()) == text.end());
}

TEST_CASE("trailing text", "[RegexTests] [match]") {
	constexpr std::string_view text = "abcdefg";
	constexpr auto lex = dpl::match{ "abc" };

	REQUIRE(lex(text.begin()) == text.begin() + 3);
}

TEST_CASE("no match", "[RegexTests] [match]") {
	constexpr std::string_view text = "abcdefg";
	constexpr auto lex = dpl::match{ "abce" };

	REQUIRE(!lex(text.begin()));
}

TEST_CASE("alts", "[RegexTests] [alternatives]") {
	constexpr std::string_view text = "jump up and down";
	constexpr auto lex = dpl::alternatives{ dpl::match{"jump"}, dpl::match{"sit"}, dpl::match{"dance"} };

	REQUIRE(lex(text.begin()) == text.begin() + 4);
}

TEST_CASE("nested", "[RegexTests] [alternatives]") {
	constexpr std::string_view text = "jump up and down";
	constexpr auto lex = dpl::alternatives{ dpl::alternatives{ dpl::match{"sit"}, dpl::match{"jump"} }, dpl::match{"dance"} };

	REQUIRE(lex(text.begin()) == text.begin() + 4);
}

TEST_CASE("seqs", "[RegexTests] [sequence]") {
	constexpr std::string_view text = "hello world!";
	constexpr auto lex = dpl::sequence{ dpl::match{"hell"}, dpl::match{"o wo"}, dpl::match{"rld!"} };

	REQUIRE(lex(text.begin()) == text.end());
}

TEST_CASE("seq of alts", "[RegexTests] [sequence]") {
	constexpr std::string_view text = "heya everyone!";
	constexpr auto lex = dpl::sequence{
		dpl::alternatives{
			dpl::match{"hello "},
			dpl::match{"heya "},
			dpl::match{"sup "}
		},
		dpl::alternatives{
			dpl::match{"world"},
			dpl::match{"people"},
			dpl::match{"everyone"}
		}
	};

	REQUIRE(lex(text.begin()) == text.end() - 1);
}

TEST_CASE("maybe", "[RegexTests] [maybe]") {
	constexpr std::string_view text1 = "hello world!";
	constexpr std::string_view text2 = "hello world.";

	constexpr auto lex = dpl::sequence{ dpl::match{"hello world"}, dpl::maybe{ dpl::match{"!"} }};

	REQUIRE(lex(text1.begin()) == text1.end());
	REQUIRE(lex(text2.begin()) == text2.end() - 1);
}

TEST_CASE("between", "[RegexTests] [between]") {
	constexpr std::string_view text1 = "xxyyyyyyyy";
	constexpr std::string_view text2 = "xxxyyyyyyy";
	constexpr std::string_view text3 = "xxxxxyyyyy";
	constexpr std::string_view text4 = "xxxxxxxyyy";
	constexpr std::string_view text5 = "xxxxxxxxxy";
	
	auto lex = dpl::between{ 3, 7, dpl::match{"x"} };

	REQUIRE(!lex(text1.begin()));
	REQUIRE(lex(text2.begin()) == text2.begin() + 3);
	REQUIRE(lex(text3.begin()) == text3.begin() + 5);
	REQUIRE(lex(text4.begin()) == text4.begin() + 7);
	REQUIRE(lex(text5.begin()) == text5.begin() + 7);
}