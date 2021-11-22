#include "catch2/catch.hpp"

#include "../src/Grammar.h"
#include "../src/TextStream.h"
#include "../src/parser/LL1.h"

#include <string_view>


using namespace std::literals::string_view_literals;
using namespace dpl::literals;

constexpr int gen_table_size() {
	auto grammar = dpl::Grammar{
		{ "Stmt", {
			{ "if"_kwd, "Expr", "then"_kwd, "Stmt" },
			{ "while"_kwd, "Expr", "do"_kwd, "Stmt" },
			{ "Expr", ";"_sym }
		}},
		{ "Expr", {
			{ "Term", "->"_sym, dpl::Token::Type::Identifier },
			{ "zero?"_sym, "Term" },
			{ "not"_kwd, "Expr" },
			{ "++"_sym, dpl::Token::Type::Identifier },
			{ "--"_sym, dpl::Token::Type::Identifier }
		}},
		{ "Term", {
			{ dpl::Token::Type::Identifier },
			{ dpl::Token::Type::Number }
		}}
	};
	dpl::LLTable table{ grammar };

	return table.size();
}

TEST_CASE("TableGen", "[constexpr]") {
	std::cout << " ===== TableGen [constexpr] =============================\n";

	constexpr int table_size = gen_table_size();
	std::cout << table_size;
	
	//REQUIRE(table_size == 6);
}