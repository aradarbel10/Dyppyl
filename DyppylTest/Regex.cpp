#include "catch2/catch.hpp"

#include "../src/Regex.h"

TEST_CASE("exact", "[RegexTests] [match]") {
	constexpr std::string_view text = "abc";
	auto lex = dpl::match{ "abc" };

	REQUIRE(lex(text.begin(), text.end()) == text.end());
}

TEST_CASE("trailing text", "[RegexTests] [match]") {
	constexpr std::string_view text = "abcdefg";
	auto lex = dpl::match{ "abc" };

	REQUIRE(lex(text.begin(), text.end()) == text.begin() + 3);
}

TEST_CASE("no match", "[RegexTests] [match]") {
	constexpr std::string_view text = "abcdefg";
	auto lex = dpl::match{ "abce" };

	REQUIRE(!lex(text.begin(), text.end()));
}

TEST_CASE("alts", "[RegexTests] [alternatives]") {
	constexpr std::string_view text = "jump up and down";
	auto lex = dpl::alternatives{ dpl::match{"jump"}, dpl::match{"sit"}, dpl::match{"dance"} };

	REQUIRE(lex(text.begin(), text.end()) == text.begin() + 4);
}

TEST_CASE("nested", "[RegexTests] [alternatives]") {
	constexpr std::string_view text = "jump up and down";
	auto lex = dpl::alternatives{ dpl::alternatives{ dpl::match{"sit"}, dpl::match{"jump"} }, dpl::match{"dance"} };

	REQUIRE(lex(text.begin(), text.end()) == text.begin() + 4);
}

TEST_CASE("seqs", "[RegexTests] [sequence]") {
	constexpr std::string_view text = "hello world!";
	auto lex = dpl::sequence{ dpl::match{"hell"}, dpl::match{"o wo"}, dpl::match{"rld!"} };

	REQUIRE(lex(text.begin(), text.end()) == text.end());
}

TEST_CASE("seq of alts", "[RegexTests] [sequence]") {
	constexpr std::string_view text = "heya everyone!";
	auto lex = dpl::sequence{
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

	REQUIRE(lex(text.begin(), text.end()) == text.end() - 1);
}

TEST_CASE("maybe", "[RegexTests] [maybe]") {
	constexpr std::string_view text1 = "hello world!";
	constexpr std::string_view text2 = "hello world.";

	auto lex = dpl::sequence{ dpl::match{"hello world"}, dpl::maybe{ dpl::match{"!"} }};

	REQUIRE(lex(text1.begin(), text1.end()) == text1.end());
	REQUIRE(lex(text2.begin(), text2.end()) == text2.end() - 1);
}

TEST_CASE("between", "[RegexTests] [between]") {
	constexpr std::string_view text1 = "xxyyyyyyyy";
	constexpr std::string_view text2 = "xxxyyyyyyy";
	constexpr std::string_view text3 = "xxxxxyyyyy";
	constexpr std::string_view text4 = "xxxxxxxyyy";
	constexpr std::string_view text5 = "xxxxxxxxxy";
	
	auto lex = dpl::between{ 3, 7, dpl::match{"x"} };

	REQUIRE(!lex(text1.begin(), text1.end()));
	REQUIRE(lex(text2.begin(), text2.end()) == text2.begin() + 3);
	REQUIRE(lex(text3.begin(), text3.end()) == text3.begin() + 5);
	REQUIRE(lex(text4.begin(), text4.end()) == text4.begin() + 7);
	REQUIRE(lex(text5.begin(), text5.end()) == text5.begin() + 7);
}

TEST_CASE("at least", "[RegexTests] [between]") {
	constexpr std::string_view text1 = "xxyyyyyyyy";
	constexpr std::string_view text2 = "xxxyyyyyyy";
	constexpr std::string_view text3 = "xxxxxyyyyy";
	constexpr std::string_view text4 = "xxxxxxxyyy";
	constexpr std::string_view text5 = "xxxxxxxxxy";

	auto lex = dpl::at_least{ 5, dpl::match{"x"} };

	REQUIRE(!lex(text1.begin(), text1.end()));
	REQUIRE(!lex(text2.begin(), text2.end()));
	REQUIRE(lex(text3.begin(), text3.end()) == text3.begin() + 5);
	REQUIRE(lex(text4.begin(), text4.end()) == text4.begin() + 7);
	REQUIRE(lex(text5.begin(), text5.end()) == text5.begin() + 9);
}

TEST_CASE("at most", "[RegexTests] [between]") {
	constexpr std::string_view text1 = "xxyyyyyyyy";
	constexpr std::string_view text2 = "xxxyyyyyyy";
	constexpr std::string_view text3 = "xxxxxyyyyy";
	constexpr std::string_view text4 = "xxxxxxxyyy";
	constexpr std::string_view text5 = "xxxxxxxxxy";

	auto lex = dpl::at_most{ 5, dpl::match{"x"} };

	REQUIRE(lex(text1.begin(), text1.end()) == text1.begin() + 2);
	REQUIRE(lex(text2.begin(), text2.end()) == text2.begin() + 3);
	REQUIRE(lex(text3.begin(), text3.end()) == text3.begin() + 5);
	REQUIRE(lex(text4.begin(), text4.end()) == text4.begin() + 5);
	REQUIRE(lex(text5.begin(), text5.end()) == text5.begin() + 5);
}

TEST_CASE("exactly", "[RegexTests] [between]") {
	constexpr std::string_view text1 = "xxyyyyyyyy";
	constexpr std::string_view text2 = "xxxyyyyyyy";
	constexpr std::string_view text3 = "xxxxxyyyyy";
	constexpr std::string_view text4 = "xxxxxxxyyy";
	constexpr std::string_view text5 = "xxxxxxxxxy";

	auto lex = dpl::exactly{ 5, dpl::match{"x"} };

	REQUIRE(!lex(text1.begin(), text1.end()));
	REQUIRE(!lex(text2.begin(), text2.end()));
	REQUIRE(lex(text3.begin(), text3.end()) == text3.begin() + 5);
	REQUIRE(lex(text4.begin(), text4.end()) == text4.begin() + 5);
	REQUIRE(lex(text5.begin(), text5.end()) == text5.begin() + 5);
}

TEST_CASE("some", "[RegexTests] [between]") {
	constexpr std::string_view text1 = "        ";
	constexpr std::string_view text2 = "+       ";
	constexpr std::string_view text3 = "++++++  ";

	auto lex = dpl::some{ dpl::match{"+"} };

	REQUIRE(!lex(text1.begin(), text1.end()));
	REQUIRE(lex(text2.begin(), text2.end()) == text2.begin() + 1);
	REQUIRE(lex(text3.begin(), text3.end()) == text3.begin() + 6);
}

TEST_CASE("kleene", "[RegexTests] [between]") {
	constexpr std::string_view text1 = "        ";
	constexpr std::string_view text2 = "*       ";
	constexpr std::string_view text3 = "******  ";

	auto lex = dpl::kleene{ dpl::match{"*"} };

	REQUIRE(lex(text1.begin(), text1.end()) == text1.begin() + 0);
	REQUIRE(lex(text2.begin(), text2.end()) == text2.begin() + 1);
	REQUIRE(lex(text3.begin(), text3.end()) == text3.begin() + 6);
}

TEST_CASE("any", "[RegexTests] [character sets]") {
	constexpr std::string_view text = "abcde12345";
	auto lex = dpl::exactly{ 5, dpl::any<char> };

	REQUIRE(lex(text.begin(), text.end()) == text.begin() + 5);
}

TEST_CASE("any of", "[RegexTests] [character sets]") {
	constexpr std::string_view text1 = "22314abc";
	constexpr std::string_view text2 = "223d4abc";

	auto lex = dpl::exactly{ 5, dpl::any_of{"1234"} };

	REQUIRE(lex(text1.begin(), text1.end()) == text1.begin() + 5);
	REQUIRE(!lex(text2.begin(), text2.end()));
}

TEST_CASE("range", "[RegexTests] [character sets]") {
	constexpr std::string_view text1 = "12345678";
	constexpr std::string_view text2 = "12945678";

	auto lex = dpl::exactly{ 4, dpl::range{'1', '4'} };

	REQUIRE(lex(text1.begin(), text1.end()) == text1.begin() + 4);
	REQUIRE(!lex(text2.begin(), text2.end()));
}

TEST_CASE("hex", "[RegexTests] [character sets]") {
	constexpr std::string_view text1 = "#5ec4ad";
	constexpr std::string_view text2 = "#3B062F";
	constexpr std::string_view text3 = "#3B*wwF";

	auto lex = dpl::sequence{
		dpl::match{"#"},
		dpl::exactly{ 6, dpl::hex_digit }
	};

	REQUIRE(lex(text1.begin(), text1.end()) == text1.end());
	REQUIRE(lex(text2.begin(), text2.end()) == text2.end());
	REQUIRE(!lex(text3.begin(), text3.end()));
}
//
//TEST_CASE("array of regex", "[RegexTests]") {
//	constexpr std::string_view text1 = "hello world!";
//
//	constexpr auto lex_word = dpl::some{ dpl::alpha };
//	constexpr auto lex_whitespace = dpl::some{ dpl::whitespace };
//	constexpr auto lex_excla = dpl::match{ "!" };
//
//	constexpr std::array<dpl::regex_wrapper<char>, 2> parts{
//		dpl::regex_wrapper<char>{ lex_word },
//		dpl::regex_wrapper<char>{ lex_word }
//	};
//}